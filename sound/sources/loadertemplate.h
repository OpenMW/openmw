#ifndef SSL_TEMPL_H
#define SSL_TEMPL_H

template <class SourceT, bool stream, bool file>
class SSL_Template : public SampleSourceLoader
{
  SSL_Template()
    {
      canLoadStream = stream;
      canLoadFile = file;
    }

  SampleSource *load(const std::string &file)
  {
    assert(canLoadFile);
    return new SourceT(file);
  }

  SampleSource *load(Stream::StreamPtr input)
  {
    assert(canLoadStream);
    return new SourceT(input);
  }
};

#endif
