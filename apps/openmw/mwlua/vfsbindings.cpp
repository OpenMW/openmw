#include "vfsbindings.hpp"

#include <sol/object.hpp>

#include <components/files/istreamptr.hpp>
#include <components/lua/luastate.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>

#include "../mwbase/environment.hpp"

#include "context.hpp"

namespace MWLua
{
    namespace
    {
        // Too many arguments may cause stack corruption and crash.
        constexpr std::size_t sMaximumReadArguments = 20;

        // Print a message if we read a large chunk of file to string.
        constexpr std::size_t sFileSizeWarningThreshold = 1024 * 1024;

        struct FileHandle
        {
        public:
            FileHandle(Files::IStreamPtr stream, std::string_view fileName)
            {
                mFilePtr = std::move(stream);
                mFileName = fileName;
            }

            Files::IStreamPtr mFilePtr;
            std::string mFileName;
        };

        std::ios_base::seekdir getSeekDir(FileHandle& self, std::string_view whence)
        {
            if (whence == "cur")
                return std::ios_base::cur;
            if (whence == "set")
                return std::ios_base::beg;
            if (whence == "end")
                return std::ios_base::end;

            throw std::runtime_error(
                "Error when handling '" + self.mFileName + "': invalid seek direction: '" + std::string(whence) + "'.");
        }

        size_t getBytesLeftInStream(Files::IStreamPtr& file)
        {
            auto oldPos = file->tellg();
            file->seekg(0, std::ios_base::end);
            auto newPos = file->tellg();
            file->seekg(oldPos, std::ios_base::beg);

            return newPos - oldPos;
        }

        void printLargeDataMessage(FileHandle& file, size_t size)
        {
            if (!file.mFilePtr || !Settings::lua().mLuaDebug || size < sFileSizeWarningThreshold)
                return;

            Log(Debug::Verbose) << "Read a large data chunk (" << size << " bytes) from '" << file.mFileName << "'.";
        }

        sol::object readFile(lua_State* lua, FileHandle& file)
        {
            std::ostringstream os;
            if (file.mFilePtr && file.mFilePtr->peek() != EOF)
                os << file.mFilePtr->rdbuf();

            auto result = os.str();
            printLargeDataMessage(file, result.size());
            return sol::make_object<std::string>(lua, std::move(result));
        }

        sol::object readLineFromFile(lua_State* lua, FileHandle& file)
        {
            std::string result;
            if (file.mFilePtr && std::getline(*file.mFilePtr, result))
            {
                printLargeDataMessage(file, result.size());
                return sol::make_object<std::string>(lua, result);
            }

            return sol::nil;
        }

        sol::object readNumberFromFile(lua_State* lua, Files::IStreamPtr& file)
        {
            double number = 0;
            if (file && *file >> number)
                return sol::make_object<double>(lua, number);

            return sol::nil;
        }

        sol::object readCharactersFromFile(lua_State* lua, FileHandle& file, size_t count)
        {
            if (count <= 0 && file.mFilePtr->peek() != EOF)
                return sol::make_object<std::string>(lua, std::string());

            auto bytesLeft = getBytesLeftInStream(file.mFilePtr);
            if (bytesLeft <= 0)
                return sol::nil;

            if (count > bytesLeft)
                count = bytesLeft;

            std::string result(count, '\0');
            if (file.mFilePtr->read(&result[0], count))
            {
                printLargeDataMessage(file, result.size());
                return sol::make_object<std::string>(lua, result);
            }

            return sol::nil;
        }

        void validateFile(const FileHandle& self)
        {
            if (self.mFilePtr)
                return;

            throw std::runtime_error("Error when handling '" + self.mFileName + "': attempt to use a closed file.");
        }

        sol::variadic_results seek(
            sol::this_state lua, FileHandle& self, std::ios_base::seekdir dir, std::streamoff off)
        {
            sol::variadic_results values;
            try
            {
                self.mFilePtr->seekg(off, dir);
                if (self.mFilePtr->fail() || self.mFilePtr->bad())
                {
                    auto msg = "Failed to seek in file '" + self.mFileName + "'";
                    values.push_back(sol::nil);
                    values.push_back(sol::make_object<std::string>(lua, msg));
                }
                else
                {
                    // tellg returns std::streampos which is not required to be a numeric type. It is required to be
                    // convertible to std::streamoff which is a number
                    values.push_back(sol::make_object<std::streamoff>(lua, self.mFilePtr->tellg()));
                }
            }
            catch (std::exception& e)
            {
                auto msg = "Failed to seek in file '" + self.mFileName + "': " + std::string(e.what());
                values.push_back(sol::nil);
                values.push_back(sol::make_object<std::string>(lua, msg));
            }

            return values;
        }
    }

    sol::table initVFSPackage(const Context& context)
    {
        sol::table api(context.mLua->unsafeState(), sol::create);

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        sol::usertype<FileHandle> fileHandle = context.sol().new_usertype<FileHandle>("FileHandle");
        fileHandle["fileName"]
            = sol::readonly_property([](const FileHandle& self) -> std::string_view { return self.mFileName; });
        fileHandle[sol::meta_function::to_string] = [](const FileHandle& self) {
            return "FileHandle{'" + self.mFileName + "'" + (!self.mFilePtr ? ", closed" : "") + "}";
        };
        fileHandle["seek"] = sol::overload(
            [](sol::this_state lua, FileHandle& self, std::string_view whence, sol::optional<long> offset) {
                validateFile(self);

                auto off = static_cast<std::streamoff>(offset.value_or(0));
                auto dir = getSeekDir(self, whence);

                return seek(lua, self, dir, off);
            },
            [](sol::this_state lua, FileHandle& self, sol::optional<long> offset) {
                validateFile(self);

                auto off = static_cast<std::streamoff>(offset.value_or(0));

                return seek(lua, self, std::ios_base::cur, off);
            });
        fileHandle["lines"] = [](sol::this_main_state lua, sol::main_object self) {
            if (!self.is<FileHandle*>())
                throw std::runtime_error("self should be a file handle");
            return sol::as_function([lua, self]() -> sol::object {
                FileHandle* handle = self.as<FileHandle*>();
                validateFile(*handle);
                return readLineFromFile(lua, *handle);
            });
        };

        api["lines"] = [vfs](sol::this_main_state lua, std::string_view fileName) {
            auto normalizedName = VFS::Path::normalizeFilename(fileName);
            return sol::as_function(
                [lua, file = FileHandle(vfs->getNormalized(normalizedName), normalizedName)]() mutable {
                    validateFile(file);
                    auto result = readLineFromFile(lua, file);
                    if (result == sol::nil)
                        file.mFilePtr.reset();

                    return result;
                });
        };

        fileHandle["close"] = [](sol::this_state lua, FileHandle& self) {
            sol::variadic_results values;
            try
            {
                self.mFilePtr.reset();
                if (self.mFilePtr)
                {
                    auto msg = "Can not close file '" + self.mFileName + "': file handle is still opened.";
                    values.push_back(sol::nil);
                    values.push_back(sol::make_object<std::string>(lua, msg));
                }
                else
                    values.push_back(sol::make_object<bool>(lua, true));
            }
            catch (std::exception& e)
            {
                auto msg = "Can not close file '" + self.mFileName + "': " + std::string(e.what());
                values.push_back(sol::nil);
                values.push_back(sol::make_object<std::string>(lua, msg));
            }

            return values;
        };

        fileHandle["read"] = [](sol::this_state lua, FileHandle& self, const sol::variadic_args args) {
            validateFile(self);

            if (args.size() > sMaximumReadArguments)
                throw std::runtime_error(
                    "Error when handling '" + self.mFileName + "': too many arguments for 'read'.");

            sol::variadic_results values;
            // If there are no arguments, read a string
            if (args.size() == 0)
            {
                values.push_back(readLineFromFile(lua, self));
                return values;
            }

            bool success = true;
            size_t i = 0;
            for (i = 0; i < args.size() && success; i++)
            {
                if (args[i].is<std::string_view>())
                {
                    auto format = args[i].as<std::string_view>();

                    if (format == "*a" || format == "*all")
                    {
                        values.push_back(readFile(lua, self));
                        continue;
                    }

                    if (format == "*n" || format == "*number")
                    {
                        auto result = readNumberFromFile(lua, self.mFilePtr);
                        values.push_back(result);
                        if (result == sol::nil)
                            success = false;
                        continue;
                    }

                    if (format == "*l" || format == "*line")
                    {
                        auto result = readLineFromFile(lua, self);
                        values.push_back(result);
                        if (result == sol::nil)
                            success = false;
                        continue;
                    }

                    throw std::runtime_error("Error when handling '" + self.mFileName + "': bad argument #"
                        + std::to_string(i + 1) + " to 'read' (invalid format)");
                }
                else if (args[i].is<int>())
                {
                    int number = args[i].as<int>();
                    auto result = readCharactersFromFile(lua, self, number);
                    values.push_back(result);
                    if (result == sol::nil)
                        success = false;
                }
            }

            // We should return nil if we just reached the end of stream
            if (!success && self.mFilePtr->eof())
                return values;

            if (!success && (self.mFilePtr->fail() || self.mFilePtr->bad()))
            {
                auto msg = "Error when handling '" + self.mFileName + "': can not read data for argument #"
                    + std::to_string(i);
                values.push_back(sol::make_object<std::string>(lua, msg));
            }

            return values;
        };

        api["open"] = [vfs](sol::this_state lua, std::string_view fileName) {
            sol::variadic_results values;
            try
            {
                auto normalizedName = VFS::Path::normalizeFilename(fileName);
                auto handle = FileHandle(vfs->getNormalized(normalizedName), normalizedName);
                values.push_back(sol::make_object<FileHandle>(lua, std::move(handle)));
            }
            catch (std::exception& e)
            {
                auto msg = "Can not open file: " + std::string(e.what());
                values.push_back(sol::nil);
                values.push_back(sol::make_object<std::string>(lua, msg));
            }

            return values;
        };

        api["type"] = sol::overload(
            [](const FileHandle& handle) -> std::string {
                if (handle.mFilePtr)
                    return "file";

                return "closed file";
            },
            [](const sol::object&) -> sol::object { return sol::nil; });

        api["fileExists"]
            = [vfs](std::string_view fileName) -> bool { return vfs->exists(VFS::Path::Normalized(fileName)); };
        api["pathsWithPrefix"] = [vfs](std::string_view prefix) {
            auto iterator = vfs->getRecursiveDirectoryIterator(prefix);
            return sol::as_function([iterator, current = iterator.begin()]() mutable -> sol::optional<std::string> {
                if (current != iterator.end())
                {
                    const std::string& result = *current;
                    ++current;
                    return result;
                }

                return sol::nullopt;
            });
        };

        return LuaUtil::makeReadOnly(api);
    }
}
