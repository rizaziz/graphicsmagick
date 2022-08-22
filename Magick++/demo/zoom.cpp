// This may look like C code, but it is really -*- C++ -*-
//
// Copyright Bob Friesenhahn, 2001, 2002, 2003
//
// Resize image using specified resize algorithm with Magick++ API
//
// Usage: zoom [-density resolution] [-filter algorithm] [-geometry geometry]
//             [-resample resolution] input_file output_file
//

#include <Magick++.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;
using namespace Magick;

//
// Some compilers (e.g. older Tru64 UNIX) lack ios::binary
//
#if defined(MISSING_STD_IOS_BINARY)
#  define IOS_IN_BINARY ios::in
#else
#  define IOS_IN_BINARY ios::in | ios::binary
#endif

static void Usage ( char **argv )
{
  cout << "Usage: " << argv[0]
       << " [-density resolution] [-filter algorithm] [-geometry geometry]"
       << " [-resample resolution] [-read-blob] input_file output_file" << endl
       << "   algorithm - bessel blackman box catrom cubic gaussian hamming hanning" << endl
       << "     hermite lanczos mitchell point quadratic sample scale sinc triangle" << endl;
  exit(1);
}

static void ParseError (int position, char **argv)
{
  cout << "Argument \"" <<  argv[position] << "\" at position" << position
       << "incorrect" << endl;
  Usage(argv);
}

int main(int argc,char **argv)
{
  // Initialize ImageMagick install location for Windows
  InitializeMagick(*argv);

  if ( argc < 2 )
    Usage(argv);

  enum ResizeAlgorithm
  {
    Zoom,
    Scale,
    Sample
  };

  {
    Geometry density;
    Geometry geometry;
    Geometry resample;
    Magick::FilterTypes filter(LanczosFilter);
    ResizeAlgorithm resize_algorithm=Zoom;
    bool read_blob = false;

    int argv_index=1;
    while ((argv_index < argc - 2) && (*argv[argv_index] == '-'))
      {
        std::string command(argv[argv_index]);
        if (command.compare("-read-blob") == 0)
          {
            read_blob = true;
            argv_index++;
            continue;
          }
        else if (command.compare("-density") == 0)
          {
            argv_index++;
            try {
              density=Geometry(argv[argv_index]);
            }
            catch( exception &/* error_ */)
              {
                ParseError(argv_index,argv);
              }
            argv_index++;
            continue;
          }
        else if (command.compare("-filter") == 0)
          {
            argv_index++;
            std::string algorithm(argv[argv_index]);
            if (algorithm.compare("point") == 0)
              filter=PointFilter;
            else if (algorithm.compare("box") == 0)
              filter=BoxFilter;
            else if (algorithm.compare("triangle") == 0)
              filter=TriangleFilter;
            else if (algorithm.compare("hermite") == 0)
              filter=HermiteFilter;
            else if (algorithm.compare("hanning") == 0)
              filter=HanningFilter;
            else if (algorithm.compare("hamming") == 0)
              filter=HammingFilter;
            else if (algorithm.compare("blackman") == 0)
              filter=BlackmanFilter;
            else if (algorithm.compare("gaussian") == 0)
              filter=GaussianFilter;
            else if (algorithm.compare("quadratic") == 0)
              filter=QuadraticFilter;
            else if (algorithm.compare("cubic") == 0)
              filter=CubicFilter;
            else if (algorithm.compare("catrom") == 0)
              filter=CatromFilter;
            else if (algorithm.compare("mitchell") == 0)
              filter=MitchellFilter;
            else if (algorithm.compare("lanczos") == 0)
              filter=LanczosFilter;
            else if (algorithm.compare("bessel") == 0)
              filter=BesselFilter;
            else if (algorithm.compare("sinc") == 0)
              filter=SincFilter;
            else if (algorithm.compare("sample") == 0)
              resize_algorithm=Sample;
            else if (algorithm.compare("scale") == 0)
              resize_algorithm=Scale;
            else
              ParseError(argv_index,argv);
            argv_index++;
            continue;
          }
        else if (command.compare("-geometry") == 0)
          {
            argv_index++;
            try {
              geometry=Geometry(argv[argv_index]);
            }
            catch( exception &/* error_ */)
              {
                ParseError(argv_index,argv);
              }
            argv_index++;
            continue;
          }
        else if (command.compare("-resample") == 0)
          {
            argv_index++;
            try {
              resample=Geometry(argv[argv_index]);
            }
            catch( exception &/* error_ */)
              {
                ParseError(argv_index,argv);
              }
            argv_index++;
            continue;
          }
        ParseError(argv_index,argv);
      }

    if (argv_index>argc-1)
      ParseError(argv_index,argv);
    std::string input_file(argv[argv_index]);
    argv_index++;
    if (argv_index>argc)
      ParseError(argv_index,argv);
    std::string output_file(argv[argv_index]);

    try {
      Image image;
      if (read_blob)
        {
          // Read image data into a blob and use image blob reader.
          Blob blob;
          string magick;
          string real_input_file = input_file;
          string::size_type idx;
          if ((idx=input_file.find(":")) != string::npos)
            {
              magick=input_file.substr(0,idx);
              real_input_file=input_file.substr(idx+1);
            }

          cout << "Magick=\"" << magick << "\", real_input_file=\"" << real_input_file << "\"" << endl;

          ifstream in( real_input_file.c_str(), IOS_IN_BINARY );
          if( !in )
          {
            cout << "Failed to open file " << input_file << " for input!" << endl;
            exit(1);
          }
          in.seekg(0,ios::end);
          streampos file_size = in.tellg();
          in.seekg(0,ios::beg);
          unsigned char* blobData = new unsigned char[file_size];
          char* c=reinterpret_cast<char *>(blobData);
          in.read(c,file_size);
          if (!in.good())
            {
              cout << "Failed to read file " << input_file << " for input!" << endl;
              exit(1);
            }
          in.close();
          cout << "Read " << file_size << " bytes from file \""
               << input_file.c_str() << "\"..." << endl;
          blob.updateNoCopy(blobData, file_size, Blob::NewAllocator );
          //blob.update(blobData, file_size);
          cout << "Reading file from blob with length " << blob.length() << endl;
          // Set image file name to whatever was provided (could have
          // magic prefix)
          image.fileName(input_file);
          //image.magick("TXT");
          image.read(blob);
          cout << "Reading file from blob" << endl;
        }
      else
        {
          // Read image directly from file
          image.read(input_file);
        }
      if (density.isValid())
        image.density(density);
      density=image.density();

      // If resample was supplied, then rescale geometry and set
      // resolution to new value.
      if (resample.isValid())
        {
          geometry =
            Geometry(static_cast<unsigned int>
                     (image.columns()*((double)resample.width()/density.width())+0.5),
                     static_cast<unsigned int>
                     (image.rows()*((double)resample.height()/density.height())+0.5));
          image.density(resample);
        }
      // Default to original geometry if it was not specified/changed
      if (!geometry.isValid())
        {
          geometry = Geometry(image.columns(),image.rows());
        }
      switch (resize_algorithm)
        {
        case Sample:
          image.sample(geometry);
          break;
        case Scale:
          image.scale(geometry);
          break;
        case Zoom:
          image.filterType(filter);
          image.zoom(geometry);
          break;
        }
      image.write(output_file);
    }
    catch( exception &error_ )
      {
        cout << "Caught exception: " << error_.what() << endl;
        return 1;
      }
  }

  return 0;
}
