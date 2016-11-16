//
// Created by koncord on 09.05.16.
//

#include <API/PublicFnAPI.hpp>
#include "LangPAWN.hpp"
#include "API/TimerAPI.hpp"

using namespace std;

cell LangPAWN::MakePublic(AMX *amx, const cell *params) noexcept
{
    int len;
    cell* source;

    source = amx_Address(amx, params[1]);
    amx_StrLen(source, &len);
    vector<char> real;
    real.reserve(len + 1);

    amx_GetString(&real[0], source, 0, UNLIMITED);

    source = amx_Address(amx, params[2]);
    amx_StrLen(source, &len);
    vector<char> name;
    name.reserve(len + 1);

    amx_GetString(&name[0], source, 0, UNLIMITED);

    cell *ret_addr = amx_Address(amx, params[3]);
    char ret_type = static_cast<char>(*reinterpret_cast<cell*>(&ret_addr));

    source = amx_Address(amx, params[4]);
    amx_StrLen(source, &len);
    vector<char> def;
    def.reserve(len + 1);

    amx_GetString(&def[0], source, 0, UNLIMITED);



    Public::MakePublic(&real[0], amx, &name[0], ret_type, &def[0]);

    return 1;
}

cell LangPAWN::CallPublic(AMX *amx, const cell *params) noexcept
{
    int len;
    cell* source;

    source = amx_Address(amx, params[1]);
    amx_StrLen(source, &len);
    vector<char> name;
    name.reserve(len + 1);

    amx_GetString(&name[0], source, 0, UNLIMITED);

    string def;

    try
    {
        def = Public::GetDefinition(&name[0]);
    }
    catch (...) { return 0; }

    vector<boost::any> args;
    unsigned int count = (params[0] / sizeof(cell)) - 1;

    if (count != def.length())
        throw runtime_error("Script call: Number of arguments does not match definition");

    for (unsigned int i = 0; i < count; ++i)
    {
        cell* data = amx_Address(amx, params[i + 2]);

        switch (def[i])
        {
            case 'i':
            {
                args.emplace_back((unsigned int) *data);
                break;
            }

            case 'q':
            {
                args.emplace_back((signed int) *data);
                break;
            }

            case 'l':
            {
                args.emplace_back((unsigned long long) *data);
                break;
            }

            case 'w':
            {
                args.emplace_back((signed long long) *data);
                break;
            }

            case 'f':
            {
                args.emplace_back((double) amx_ctof(*data));
                break;
            }

            case 'p':
            {
                args.emplace_back((void*) data);
                break;
            }

            case 's':
            {
                amx_StrLen(data, &len);
                vector<char> str;
                str.reserve(len + 1);
                amx_GetString(&str[0], data, 0, UNLIMITED);
                args.emplace_back(string(&str[0]).c_str());
                break;
            }

            default:
                throw runtime_error("PAWN call: Unknown argument identifier " + def[i]);
        }
    }

    boost::any result = Public::Call(&name[0], args);
    if (result.empty())
        return 0;

    cell ret = 0;

    if (result.type().hash_code() == typeid(signed int).hash_code())
        ret = boost::any_cast<signed int>(result);
    else if (result.type().hash_code() == typeid(unsigned int).hash_code())
        ret = boost::any_cast<unsigned int>(result);
    else if (result.type().hash_code() == typeid(double).hash_code())
        ret = amx_ftoc(result);

    return ret;
}

cell LangPAWN::CreateTimer(AMX *amx, const cell *params) noexcept
{

}

cell LangPAWN::CreateTimerEx(AMX *amx, const cell *params) noexcept
{

}