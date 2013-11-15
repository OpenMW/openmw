#ifndef UNSHIELD_THREAD_H
#define UNSHIELD_THREAD_H

#include <QThread>

#include <boost/filesystem.hpp>

#include <libunshield.h>

namespace Launcher
{
    class UnshieldThread : public QThread
    {
       Q_OBJECT

        public:
            bool SetMorrowindPath(const std::string& path);
            bool SetTribunalPath(const std::string& path);
            bool SetBloodmoonPath(const std::string& path);

            void SetOutputPath(const std::string& path);

            bool extract();

            bool TribunalDone();
            bool BloodmoonDone();

            void Done();

            std::string GetMWEsmPath();

            UnshieldThread();

        private:

            void extract_cab(const boost::filesystem::path& cab, const boost::filesystem::path& output_dir, bool extract_ini = false);
            bool extract_file(Unshield* unshield, boost::filesystem::path output_dir, const char* prefix, int index);

            boost::filesystem::path mMorrowindPath;
            boost::filesystem::path mTribunalPath;
            boost::filesystem::path mBloodmoonPath;

            bool mMorrowindDone;
            bool mTribunalDone;
            bool mBloodmoonDone;

            boost::filesystem::path mOutputPath;


        protected:
            virtual void run();

        signals:
            void signalGUI(QString);
            void close();
    };
}
#endif
