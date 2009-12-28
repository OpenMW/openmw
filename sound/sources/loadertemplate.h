#ifndef SSL_TEMPL_H
#define SSL_TEMPL_H

template <class X, bool stream, bool file>
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
    return new X(file);
  }

  SampleSource *load(Stream::Stream *input)
  {
    assert(canLoadStream);
    return new X(input);
  }
};

#endif
