
#include <cstring>

#include <v8.h>

#include <node.h>
#include <node_buffer.h>

#include <magick/api.h>

#define THROW_ERROR(TYPE, STR)                                          \
  return ThrowException(Exception::TYPE(String::New(STR)));

#define REQ_ARGS(N)                                                     \
  if (args.Length() < (N))                                              \
    return ThrowException(Exception::TypeError(                         \
                             String::New("Expected " #N "arguments")));

#define REQ_STR_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsString())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a string")));    \
  String::Utf8Value VAR(args[I]->ToString());

#define REQ_INT_ARG(I, VAR)                                             \
  int VAR;                                                              \
  if (args.Length() <= (I) || !args[I]->IsInt32())                      \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be an integer")));  \
  VAR = args[I]->Int32Value();


#define REQ_IMG_ARG(I, VAR) \
if (args.Length() <= (I) || !args[I]->IsObject())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be an object")));   \
  Local<Object> _obj_ = Local<Object>::Cast(args[I]);                   \
  MagickImage *VAR = ObjectWrap::Unwrap<MagickImage>(_obj_);

#define OPT_INT_ARG(I, VAR, DEFAULT)                                    \
  int VAR;                                                              \
  if (args.Length() <= (I)) {                                           \
    VAR = (DEFAULT);                                                    \
  } else if (args[I]->IsInt32()) {                                      \
    VAR = args[I]->Int32Value();                                        \
  } else {                                                              \
    return ThrowException(Exception::TypeError(                         \
              String::New("Argument " #I " must be an integer"))); \
  }


#define REQ_RECT_ARG(I, VAR)                                            \
  REQ_INT_ARG(I+0, x)                       \
  REQ_INT_ARG(I+1, y)                     \
  REQ_INT_ARG(I+2, width)                         \
  REQ_INT_ARG(I+3, height)                          \
  RectangleInfo VAR = { width, height, x, y };

#define REQ_DOUBLE_ARG(I, VAR)                                          \
  double VAR;                                                           \
  if (args.Length() <= (I) || !args[I]->IsNumber())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a number")));    \
  VAR = args[I]->NumberValue();

#define REQ_EXT_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsExternal())                   \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " invalid")));             \
  Local<External> VAR = Local<External>::Cast(args[I]);


#define OPT_INT_ARG(I, VAR, DEFAULT)                                    \
  int VAR;                                                              \
  if (args.Length() <= (I)) {                                           \
    VAR = (DEFAULT);                                                    \
  } else if (args[I]->IsInt32()) {                                      \
    VAR = args[I]->Int32Value();                                        \
  } else {                                                              \
    return ThrowException(Exception::TypeError(                         \
              String::New("Argument " #I " must be an integer")));    \
  }

#define OPT_DOUBLE_ARG(I, VAR, DEFAULT)                                    \
  int VAR;                                                              \
  if (args.Length() <= (I)) {                                           \
    VAR = (DEFAULT);                                                    \
  } else if (args[I]->IsNumber()) {                                      \
    VAR = args[I]->NumberValue();                                        \
  } else {                                                              \
    return ThrowException(Exception::TypeError(                         \
              String::New("Argument " #I " must be a number")));    \
  }

using namespace node;
using namespace v8;

/*
template <typename T> Handle<T> returnNewPointer(T ptr) {
  HandleScope scope;
  Local<Value> arg = External::New(ptr);
  Persistent<Object> obj(T::Constructor->GetFunction()->NewInstance(1, &arg));
  return scope.Close(obj);
}
*/


#define IMAGE_METHOD(apiname, apiargs...) \
    HandleScope scope; \
    Handle<Value> out; \
    ExceptionInfo exception; \
    Image *result; \
    MagickImage *image = ObjectWrap::Unwrap<MagickImage>(args.This()); \
    GetExceptionInfo(&exception); \
    result = apiname( *image, ##apiargs, &exception ); \
    if (result) { \
      Local<Object> object = constructorTemplate->GetFunction()->NewInstance(); \
      MagickImage *magickImage = ObjectWrap::Unwrap<MagickImage>(object); \
      magickImage->image = result; \
      out = scope.Close(object); \
    } else { \
      CatchException(&exception); \
      out = ThrowException(String::New("Unable to load image!")); \
    } \
    DestroyExceptionInfo(&exception); \
    return out;

class MagickImage : ObjectWrap {
public:

  static Persistent<FunctionTemplate> constructorTemplate;

  Image* image;
  char *_format;
  size_t length;
  int _quality;



  MagickImage(Image* i) : ObjectWrap(), image(i) {
    _format = NULL;
    _quality = 90;
  }

  ~MagickImage() {
    free(_format);
    if (image) DestroyImage(image);
  }

  operator Image* () const {
    return image;
  }



  static void Init(Handle<Object> target) {


    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    //Create a new persistent function template based around "create"; this
    //template is used as the prototype for making new instances of the object
    constructorTemplate = Persistent<FunctionTemplate>::New(t);

    //This object has one internal field (i.e. a field hidden from javascript);
    //This field is used to store a pointer to the image class
    constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

    //Give the class a name
    constructorTemplate->SetClassName(String::NewSymbol("Image"));

    //All the methods for this class
    NODE_SET_PROTOTYPE_METHOD(t, "thumbnail", thumbnail);
    NODE_SET_PROTOTYPE_METHOD(t, "sample", sample);
    NODE_SET_PROTOTYPE_METHOD(t, "scale", scale);
    NODE_SET_PROTOTYPE_METHOD(t, "resize", resize);
    NODE_SET_PROTOTYPE_METHOD(t, "chop", chop);
    NODE_SET_PROTOTYPE_METHOD(t, "crop", crop);
    NODE_SET_PROTOTYPE_METHOD(t, "extent", extent);
    NODE_SET_PROTOTYPE_METHOD(t, "flip", flip);
    NODE_SET_PROTOTYPE_METHOD(t, "flop", flop);
    NODE_SET_PROTOTYPE_METHOD(t, "affineTransform", affineTransform);
    NODE_SET_PROTOTYPE_METHOD(t, "rotate", rotate);
    NODE_SET_PROTOTYPE_METHOD(t, "format", format);
    NODE_SET_PROTOTYPE_METHOD(t, "quality", quality);
    NODE_SET_PROTOTYPE_METHOD(t, "shear", shear);
    NODE_SET_PROTOTYPE_METHOD(t, "contrast", contrast);
    NODE_SET_PROTOTYPE_METHOD(t, "equalize", equalize);
    NODE_SET_PROTOTYPE_METHOD(t, "gamma", gamma);
    NODE_SET_PROTOTYPE_METHOD(t, "level", level);
    NODE_SET_PROTOTYPE_METHOD(t, "levelChannel", levelChannel);
    NODE_SET_PROTOTYPE_METHOD(t, "modulate", modulate);
    NODE_SET_PROTOTYPE_METHOD(t, "negate", negate);
    NODE_SET_PROTOTYPE_METHOD(t, "normalize", normalize);
    NODE_SET_PROTOTYPE_METHOD(t, "attribute", attribute);
    NODE_SET_PROTOTYPE_METHOD(t, "composite", composite);

    //Some getters
    t->PrototypeTemplate()->SetAccessor(String::NewSymbol("buffer"), getBuffer, NULL, Handle<Value>(), PROHIBITS_OVERWRITING, ReadOnly);
    t->PrototypeTemplate()->SetAccessor(String::NewSymbol("width"), getWidth, NULL, Handle<Value>(), PROHIBITS_OVERWRITING, ReadOnly);
    t->PrototypeTemplate()->SetAccessor(String::NewSymbol("height"), getHeight, NULL, Handle<Value>(), PROHIBITS_OVERWRITING, ReadOnly);
  }

  static Handle<Value> New(const Arguments &args) {
    HandleScope scope;
    MagickImage* magickImage = new MagickImage(NULL);
    magickImage->Wrap(args.This());
    return args.This();
  }


  static Handle<Value> getBuffer (Local<String> property, const AccessorInfo& info)
  {
    HandleScope scope;
    ExceptionInfo exception;
    size_t length;
    ImageInfo *imageInfo = CloneImageInfo(NULL);
    MagickImage *image = ObjectWrap::Unwrap<MagickImage>(info.This());
    GetExceptionInfo(&exception);
    strcpy(imageInfo->filename, "");
    Image *img = *image;
    imageInfo->quality = image->_quality;
    strcpy(img->magick, image->_format);
    StripImage(img);
    void* data = ImageToBlob(imageInfo, *image, &length, &exception);
    if (data) {
      //http://sambro.is-super-awesome.com/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
      Buffer *slowBuffer = Buffer::New(length);
      memcpy(Buffer::Data(slowBuffer), data, length);
      Local<Object> globalObj = Context::GetCurrent()->Global();
      Local<Function> bufferConstructor = Local<Function>::Cast(globalObj->Get(String::New("Buffer")));
      Handle<Value> constructorArgs[3] = { slowBuffer->handle_, Integer::New(length), Integer::New(0) };
      Local<Object> buffer =  bufferConstructor->NewInstance(3, constructorArgs);
      free(data);
      DestroyImageInfo(imageInfo);
      return scope.Close(buffer);
    } else {
      DestroyImageInfo(imageInfo);
      return ThrowException(String::New("Unable to convert image to blob!"));
    }


  }

  static Handle<Value> getWidth (Local<String> property, const AccessorInfo& info)
  {
    HandleScope scope;
    MagickImage* image = ObjectWrap::Unwrap<MagickImage>(info.This());
    Local<Number> result = Integer::New(image->image->columns);
    return scope.Close(result);
  }

  static Handle<Value> getHeight (Local<String> property, const AccessorInfo& info)
  {
    HandleScope scope;
    MagickImage* image = ObjectWrap::Unwrap<MagickImage>(info.This());
    Local<Number> result = Integer::New(image->image->rows);
    return scope.Close(result);
  }

  static Handle<Value> info(const Arguments &args) {

  }

  /**
   * Create a new image from a buffer
   */
  static Handle<Value> create(const Arguments &args) {

    HandleScope scope;

    if (args.Length() < 1) {
      return Undefined();
    }

    Handle<Value> result;
    ExceptionInfo exception;
    const char* blob;
    size_t length;
    Image* image;
    ImageInfo *imageInfo = CloneImageInfo(NULL);

    GetExceptionInfo(&exception);

    if (args[0]->IsString()) {
      String::AsciiValue string(args[0]->ToString());
      length = string.length();
      blob = *string;
    } else if (Buffer::HasInstance(args[0])) {
      Local<Object> bufferIn=args[0]->ToObject();
      length = Buffer::Length(bufferIn);
      blob = Buffer::Data(bufferIn);
    }

    image = BlobToImage(imageInfo, blob, length, &exception);
    if (!image) {
       CatchException(&exception);
       result = ThrowException(String::New("Unable to load image!"));
    }
    else {
      Local<Object> object = constructorTemplate->GetFunction()->NewInstance();
      MagickImage *magickImage = ObjectWrap::Unwrap<MagickImage>(object);
      magickImage->image = image;
      magickImage->length = length;
      result = scope.Close(object);
    }

    DestroyImageInfo(imageInfo);
    DestroyExceptionInfo(&exception);
    return result;
  }

  static Handle<Value> thumbnail(const Arguments &args) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    IMAGE_METHOD(ThumbnailImage, width, height)
  }

  static Handle<Value> sample(const Arguments &args) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    IMAGE_METHOD(SampleImage, width, height)
  }

  static Handle<Value> scale(const Arguments &args) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    IMAGE_METHOD(ScaleImage, width, height)
  }

  static Handle<Value> resize(const Arguments &args) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    OPT_INT_ARG(2, f, LanczosFilter)
    OPT_DOUBLE_ARG(3, blur, 1.0)
    FilterTypes filter = FilterTypes(f);
    IMAGE_METHOD(ResizeImage, width, height, filter, blur)

  }

  //http://www.graphicsmagick.org/api/transform.html#chopimage
  static Handle<Value> chop(const Arguments &args) {
    REQ_RECT_ARG(0, chopInfo)
    IMAGE_METHOD(ChopImage, &chopInfo)
  }

  //http://www.graphicsmagick.org/api/transform.html#cropimage
  static Handle<Value> crop(const Arguments &args) {
    REQ_RECT_ARG(0, cropInfo)
    IMAGE_METHOD(CropImage, &cropInfo)
  }

  //http://www.graphicsmagick.org/api/transform.html#extentimage
  static Handle<Value> extent(const Arguments &args) {
    REQ_RECT_ARG(0, geometry)
    IMAGE_METHOD(ExtentImage, &geometry)
  }

  static Handle<Value> flip(const Arguments &args) {
    IMAGE_METHOD(FlipImage)
  }

  static Handle<Value> flop(const Arguments &args) {
    IMAGE_METHOD(FlopImage)
  }

  //http://www.graphicsmagick.org/api/shear.html#affinetransformimage
  static Handle<Value> affineTransform(const Arguments &args) {
    AffineMatrix affineMatrix;
    IMAGE_METHOD(AffineTransformImage, &affineMatrix)
  }

  //http://www.graphicsmagick.org/api/shear.html#rotateimage
  static Handle<Value> rotate(const Arguments &args) {
    REQ_DOUBLE_ARG(0, degrees)
    IMAGE_METHOD(RotateImage, degrees)
  }

  static Handle<Value> quality(const Arguments &args) {
    HandleScope scope;
    REQ_DOUBLE_ARG(0, quality)
    MagickImage *image = ObjectWrap::Unwrap<MagickImage>(args.This());
    image->_quality = quality;
    return Undefined();
  }

  static Handle<Value> format(const Arguments &args) {
    HandleScope scope;
    REQ_STR_ARG(0, format)
    MagickImage *image = ObjectWrap::Unwrap<MagickImage>(args.This());
    image->_format = strdup(*format);
    return Undefined();
  }

  //http://www.graphicsmagick.org/api/shear.html#shearimage
  static Handle<Value> shear(const Arguments &args) {
    REQ_DOUBLE_ARG(0, x)
    REQ_DOUBLE_ARG(1, y)
    IMAGE_METHOD(ShearImage, x, y)
  }

  //http://www.graphicsmagick.org/api/enhance.html#contrastimage
  static Handle<Value> contrast(const Arguments &args) {
    REQ_INT_ARG(0, s)
    //IMAGE_METHOD(ContrastImage, s)
    return Undefined();
  }

  //http://www.graphicsmagick.org/api/enhance.html#equalizeimage
  static Handle<Value> equalize(const Arguments &args) {
    //IMAGE_METHOD(EqualizeImage)
    return Undefined();
  }

  static Handle<Value> gamma(const Arguments &args) {
    //IMAGE_METHOD(GammaImage)
    return Undefined();
  }

  static Handle<Value> level(const Arguments &args) {
    //IMAGE_METHOD(LevelImage)
    return Undefined();
  }

  static Handle<Value> levelChannel(const Arguments &args) {
    //IMAGE_METHOD(LevelImageChannel)
    return Undefined();
  }

  static Handle<Value> modulate(const Arguments &args) {
    //IMAGE_METHOD(ModulateImage)
    return Undefined();
  }

  static Handle<Value> negate(const Arguments &args) {
    //IMAGE_METHOD(NegateImage)
    return Undefined();
  }

  static Handle<Value> normalize(const Arguments &args) {
    //IMAGE_METHOD(NormalizeImage)
    return Undefined();
  }

  //http://www.graphicsmagick.org/api/attribute.html#getimageattribute
  //http://www.graphicsmagick.org/api/attribute.html#setimageattribute
  static Handle<Value> attribute(const Arguments &args) {
    //MagickImage *image = ObjectWrap::Unwrap<MagickImage>(args.This());
    //ExceptionInfo exception;
    return Undefined();
  }

  //http://www.graphicsmagick.org/api/composite.html
  static Handle<Value> composite(const Arguments &args) {
    HandleScope scope;
    Handle<Value> out;
    CompositeOperator compose;
    MagickImage *image = ObjectWrap::Unwrap<MagickImage>(args.This());
    REQ_IMG_ARG(0, i)
    REQ_INT_ARG(1, c)
    OPT_INT_ARG(2, x, 0)
    OPT_INT_ARG(3, y, 0)
    const Image* compositeImage = *i;
    compose = CompositeOperator(c);
    if (CompositeImage( *image, compose, compositeImage, x, y ) == MagickPass)
      return args.This();
    else
      return ThrowException(String::New("Unable to composite image!"));

  }

};

Persistent<FunctionTemplate> MagickImage::constructorTemplate;

extern "C" {
  static void init (Handle<Object> target)
  {
    InitializeMagick(NULL);


    //http://www.graphicsmagick.org/api/types.html#filtertypes
    NODE_DEFINE_CONSTANT(target, UndefinedFilter);
    NODE_DEFINE_CONSTANT(target, PointFilter);
    NODE_DEFINE_CONSTANT(target, BoxFilter);
    NODE_DEFINE_CONSTANT(target, TriangleFilter);
    NODE_DEFINE_CONSTANT(target, HermiteFilter);
    NODE_DEFINE_CONSTANT(target, HanningFilter);
    NODE_DEFINE_CONSTANT(target, HammingFilter);
    NODE_DEFINE_CONSTANT(target, BlackmanFilter);
    NODE_DEFINE_CONSTANT(target, GaussianFilter);
    NODE_DEFINE_CONSTANT(target, QuadraticFilter);
    NODE_DEFINE_CONSTANT(target, CubicFilter);
    NODE_DEFINE_CONSTANT(target, CatromFilter);
    NODE_DEFINE_CONSTANT(target, MitchellFilter);
    NODE_DEFINE_CONSTANT(target, LanczosFilter);
    NODE_DEFINE_CONSTANT(target, BesselFilter);
    NODE_DEFINE_CONSTANT(target, SincFilter);

    //http://www.graphicsmagick.org/api/types.html#compositeoperator
    NODE_DEFINE_CONSTANT(target, UndefinedCompositeOp);
    NODE_DEFINE_CONSTANT(target, OverCompositeOp);
    NODE_DEFINE_CONSTANT(target, InCompositeOp);
    NODE_DEFINE_CONSTANT(target, OutCompositeOp);
    NODE_DEFINE_CONSTANT(target, AtopCompositeOp);
    NODE_DEFINE_CONSTANT(target, XorCompositeOp);
    NODE_DEFINE_CONSTANT(target, PlusCompositeOp);
    NODE_DEFINE_CONSTANT(target, MinusCompositeOp);
    NODE_DEFINE_CONSTANT(target, AddCompositeOp);
    NODE_DEFINE_CONSTANT(target, SubtractCompositeOp);
    NODE_DEFINE_CONSTANT(target, DifferenceCompositeOp);
    NODE_DEFINE_CONSTANT(target, BumpmapCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyRedCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyGreenCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyBlueCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyOpacityCompositeOp);
    NODE_DEFINE_CONSTANT(target, ClearCompositeOp);
    NODE_DEFINE_CONSTANT(target, DissolveCompositeOp);
    NODE_DEFINE_CONSTANT(target, DisplaceCompositeOp);
    NODE_DEFINE_CONSTANT(target, ModulateCompositeOp);
    NODE_DEFINE_CONSTANT(target, ThresholdCompositeOp);
    NODE_DEFINE_CONSTANT(target, NoCompositeOp);
    NODE_DEFINE_CONSTANT(target, DarkenCompositeOp);
    NODE_DEFINE_CONSTANT(target, LightenCompositeOp);
    NODE_DEFINE_CONSTANT(target, HueCompositeOp);
    NODE_DEFINE_CONSTANT(target, SaturateCompositeOp);
    NODE_DEFINE_CONSTANT(target, ColorizeCompositeOp);
    NODE_DEFINE_CONSTANT(target, LuminizeCompositeOp);
    NODE_DEFINE_CONSTANT(target, ScreenCompositeOp);
    NODE_DEFINE_CONSTANT(target, OverlayCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyCyanCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyMagentaCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyYellowCompositeOp);
    NODE_DEFINE_CONSTANT(target, CopyBlackCompositeOp);
    NODE_DEFINE_CONSTANT(target, DivideCompositeOp);

    NODE_SET_METHOD(target, "image", MagickImage::create);

    MagickImage::Init(target);
  }

  NODE_MODULE(GraphicsMagick, init)
}
