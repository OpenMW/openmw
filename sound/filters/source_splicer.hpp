#ifndef MANGLE_SOUND_SOURCE_SPLICE_H
#define MANGLE_SOUND_SOURCE_SPLICE_H

#include "../source.hpp"
#include "../../tools/str_exception.hpp"
#include <string>
#include <list>
#include <assert.h>

namespace Mangle
{
  namespace Sound
  {
    class SourceSplicer : public SampleSourceLoader
    {
      struct SourceType
      {
        std::string type;
        SampleSourceLoaderPtr loader;
      };

      typedef std::list<SourceType> TypeList;
      TypeList list;
      SampleSourceLoaderPtr catchAll;

      static bool isMatch(char a, char b)
      {
        if(a >= 'A' && a <= 'Z')
          a += 'a' - 'A';
        if(b >= 'A' && b <= 'Z')
          b += 'a' - 'A';
        return a == b;
      }

    public:
      SourceSplicer()
      {
        canLoadStream = false;
        canLoadFile = true;
      }

      void add(const std::string &type, SampleSourceLoaderPtr fact)
      {
        SourceType tp;
        tp.type = type;
        tp.loader = fact;
        list.push_back(tp);
      }

      void setDefault(SampleSourceLoaderPtr def)
      {
        catchAll = def;
      }

      SampleSourcePtr load(const std::string &file)
      {
        // Search the list for this file type.
        for(TypeList::iterator it = list.begin();
            it != list.end(); it++)
          {
            const std::string &t = it->type;

            int diff = file.size() - t.size();
            if(diff < 0) continue;

            bool match = true;
            for(int i=0; i<t.size(); i++)
              if(!isMatch(t[i], file[i+diff]))
                {
                  match = false;
                  break;
                }

            // Got something! We're done.
            if(match)
              return it->loader->load(file);
          }
        // If not found, use the catch-all
        if(catchAll)
          return catchAll->load(file);

        throw str_exception("No handler for sound file " + file);
      }

      SampleSourcePtr load(Stream::StreamPtr input) { assert(0); }
    };
  }
}

#endif
