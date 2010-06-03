#ifndef SSL_TEMPL_H
#define SSL_TEMPL_H

template <class SourceT, bool stream, bool file>
class SSL_Template : public SampleSourceLoader
{
 public:

  SSL_Template()
    {
      canLoadStream = stream;
      canLoadFile = file;
    }

  SampleSourcePtr load(const std::string &filename)
  {
    assert(canLoadFile);
    return SampleSourcePtr(new SourceT(filename));
  }

  SampleSourcePtr load(Stream::StreamPtr input)
  {
    assert(canLoadStream);
    return SampleSourcePtr(new SourceT(input));
  }
};

#endif
