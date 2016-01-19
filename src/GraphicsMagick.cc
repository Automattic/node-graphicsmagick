#include <cstring>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>

#include <magick/api.h>

#define REQ_ARGS(N)                                                     \
  if (info.Length() < (N))                                              \
    return Nan::ThrowTypeError(("Expected " #N "arguments"));

#define REQ_STR_ARG(I, VAR)                                             \
  if (info.Length() <= (I) || !info[I]->IsString())                     \
    return Nan::ThrowTypeError(("Argument " #I " must be a string"));   \
  Nan::Utf8String VAR(info[I]->ToString());

#define REQ_INT_ARG(I, VAR)                                             \
  int VAR;                                                              \
  if (info.Length() <= (I) || !info[I]->IsInt32())                      \
    return Nan::ThrowTypeError(("Argument " #I " must be an integer")); \
  VAR = Nan::To<int32_t>(info[I]).FromJust();

#define REQ_IMG_ARG(I, VAR) \
  if (info.Length() <= (I) || !info[I]->IsObject())                    \
    return Nan::ThrowTypeError(("Argument " #I " must be an object")); \
  Handle<Object> _obj_ = Handle<Object>::Cast(info[I]);                \
  MagickImage *VAR = Nan::ObjectWrap::Unwrap<MagickImage>(_obj_);

#define REQ_RECT_ARG(I, VAR)                                            \
  REQ_INT_ARG(I+0, x)                                                   \
  REQ_INT_ARG(I+1, y)                                                   \
  REQ_INT_ARG(I+2, width)                                               \
  REQ_INT_ARG(I+3, height)                                              \
  RectangleInfo VAR = {                                                 \
    static_cast<unsigned long>(width),                                  \
    static_cast<unsigned long>(height),                                 \
    static_cast<long>(x),                                               \
    static_cast<long>(x),                                               \
  };

#define REQ_DOUBLE_ARG(I, VAR)                                          \
  double VAR;                                                           \
  if (info.Length() <= (I) || !info[I]->IsNumber())                     \
    return Nan::ThrowTypeError(("Argument " #I " must be a number"));   \
  VAR = Nan::To<int64_t>(info[I]).FromJust();

#define REQ_EXT_ARG(I, VAR)                                             \
  if (info.Length() <= (I) || !info[I]->IsExternal())                   \
    return Nan::ThrowTypeError(("Argument " #I " invalid"));            \
  Handle<External> VAR = Handle<External>::Cast(info[I]);

#define OPT_INT_ARG(I, VAR, DEFAULT)                                    \
  int VAR;                                                              \
  if (info.Length() <= (I)) {                                           \
    VAR = (DEFAULT);                                                    \
  } else if (info[I]->IsInt32()) {                                      \
    VAR = Nan::To<int32_t>(info[I]).FromJust();                         \
  } else {                                                              \
    return Nan::ThrowTypeError(("Argument " #I " must be an integer")); \
  }

#define OPT_DOUBLE_ARG(I, VAR, DEFAULT)                                 \
  int VAR;                                                              \
  if (info.Length() <= (I)) {                                           \
    VAR = (DEFAULT);                                                    \
  } else if (info[I]->IsNumber()) {                                     \
    VAR = Nan::To<int64_t>(info[I]).FromJust();                         \
  } else {                                                              \
    return Nan::ThrowTypeError(("Argument " #I " must be a number"));   \
  }

using namespace node;
using namespace node::Buffer;
using namespace v8;


#define IMAGE_METHOD(apiname, apiargs...) \
    Nan::HandleScope scope; \
    ExceptionInfo exception; \
    Image *result; \
    MagickImage *image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This()); \
    GetExceptionInfo(&exception); \
    result = apiname( *image, ##apiargs, &exception ); \
    if (result) { \
      Handle<Object> object = Nan::NewInstance(Nan::GetFunction(Nan::New<FunctionTemplate>(constructorTemplate)).ToLocalChecked()).ToLocalChecked(); \
      MagickImage *magickImage = Nan::ObjectWrap::Unwrap<MagickImage>(object); \
      magickImage->image = result; \
      info.GetReturnValue().Set(object); \
    } else { \
      CatchException(&exception); \
      return Nan::ThrowError("Unable to load image!"); \
    } \
    DestroyExceptionInfo(&exception);

class MagickImage : public Nan::ObjectWrap {
public:

  static Nan::Persistent<FunctionTemplate> constructorTemplate;

  Image* image;
  char *_format;
  size_t length;
  int _quality;



  MagickImage(Image* i) : Nan::ObjectWrap(), image(i) {
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


  static NAN_MODULE_INIT(Init) {
    Handle<FunctionTemplate> t = Nan::New<FunctionTemplate>(New);

    //Create a new persistent function template based around "create"; this
    //template is used as the prototype for making new instances of the object
    constructorTemplate.Reset(t);

    //This object has one internal field (i.e. a field hidden from javascript);
    //This field is used to store a pointer to the image class
    t->InstanceTemplate()->SetInternalFieldCount(1);

    //Give the class a name
    t->SetClassName(Nan::New<v8::String>("Image").ToLocalChecked());

    //All the methods for this class
    Nan::SetPrototypeMethod(t, "thumbnail", thumbnail);
    Nan::SetPrototypeMethod(t, "sample", sample);
    Nan::SetPrototypeMethod(t, "scale", scale);
    Nan::SetPrototypeMethod(t, "resize", resize);
    Nan::SetPrototypeMethod(t, "chop", chop);
    Nan::SetPrototypeMethod(t, "crop", crop);
    Nan::SetPrototypeMethod(t, "extent", extent);
    Nan::SetPrototypeMethod(t, "flip", flip);
    Nan::SetPrototypeMethod(t, "flop", flop);
    Nan::SetPrototypeMethod(t, "affineTransform", affineTransform);
    Nan::SetPrototypeMethod(t, "rotate", rotate);
    Nan::SetPrototypeMethod(t, "format", format);
    Nan::SetPrototypeMethod(t, "quality", quality);
    Nan::SetPrototypeMethod(t, "shear", shear);
    Nan::SetPrototypeMethod(t, "contrast", contrast);
    Nan::SetPrototypeMethod(t, "equalize", equalize);
    Nan::SetPrototypeMethod(t, "gamma", gamma);
    Nan::SetPrototypeMethod(t, "level", level);
    Nan::SetPrototypeMethod(t, "levelChannel", levelChannel);
    Nan::SetPrototypeMethod(t, "modulate", modulate);
    Nan::SetPrototypeMethod(t, "negate", negate);
    Nan::SetPrototypeMethod(t, "normalize", normalize);
    Nan::SetPrototypeMethod(t, "attribute", attribute);
    Nan::SetPrototypeMethod(t, "composite", composite);

    //Some getters
    Nan::SetAccessor(t->PrototypeTemplate(), Nan::New<v8::String>("buffer").ToLocalChecked(), getBuffer);
    Nan::SetAccessor(t->PrototypeTemplate(), Nan::New<v8::String>("width").ToLocalChecked(), getWidth);
    Nan::SetAccessor(t->PrototypeTemplate(), Nan::New<v8::String>("height").ToLocalChecked(), getHeight);
  }

  static NAN_METHOD(New) {
    Nan::HandleScope scope;
    MagickImage* magickImage = new MagickImage(NULL);
    magickImage->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }


  static void getBuffer(v8::Handle<v8::String> property,
                              const Nan::PropertyCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;
    ExceptionInfo exception;
    size_t length;
    ImageInfo *imageInfo = CloneImageInfo(NULL);
    MagickImage *image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    GetExceptionInfo(&exception);
    strcpy(imageInfo->filename, "");
    Image *img = *image;
    imageInfo->quality = image->_quality;
    strcpy(img->magick, image->_format);
    StripImage(img);
    void* data = ImageToBlob(imageInfo, *image, &length, &exception);
    if (data) {
      //http://sambro.is-super-awesome.com/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
      Handle<Object> buffer = Nan::NewBuffer(length).ToLocalChecked();
      memcpy(Buffer::Data(buffer), data, length);
      info.GetReturnValue().Set(buffer);
      free(data);
      DestroyImageInfo(imageInfo);
    } else {
      DestroyImageInfo(imageInfo);
      return Nan::ThrowError("Unable to convert image to blob!");
    }
  }

  static void getWidth(v8::Handle<v8::String> property,
                              const Nan::PropertyCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;
    MagickImage* image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    Handle<Number> result = Nan::New<Number>(image->image->columns);
    info.GetReturnValue().Set(result);
  }

  static void getHeight(v8::Handle<v8::String> property,
                              const Nan::PropertyCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;
    MagickImage* image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    Handle<Number> result = Nan::New<Number>(image->image->rows);
    info.GetReturnValue().Set(result);
  }

  /**
   * Create a new image from a buffer
   */
  static void create(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;

    if (info.Length() < 1) {
      info.GetReturnValue().SetUndefined();
      return;
    }

    Handle<Value> result;
    ExceptionInfo exception;
    const char* blob = NULL;
    size_t length = 0;
    Image* image;
    ImageInfo *imageInfo = CloneImageInfo(NULL);

    GetExceptionInfo(&exception);

    if (info[0]->IsString()) {
      return Nan::ThrowTypeError("binary string input is no longer supported");
      //String::AsciiValue string(info[0]->ToString());
      //length = string.length();
      //blob = *string;
    } else if (Buffer::HasInstance(info[0])) {
      Handle<Object> bufferIn = info[0]->ToObject();
      length = Buffer::Length(bufferIn);
      blob = Buffer::Data(bufferIn);
    }

    image = BlobToImage(imageInfo, blob, length, &exception);
    if (!image) {
       CatchException(&exception);
       return Nan::ThrowError("Unable to load image!");
    }
    else {
      Handle<Object> object = Nan::NewInstance(Nan::GetFunction(Nan::New<FunctionTemplate>(constructorTemplate)).ToLocalChecked()).ToLocalChecked();
      MagickImage *magickImage = Nan::ObjectWrap::Unwrap<MagickImage>(object);
      magickImage->image = image;
      magickImage->length = length;
      info.GetReturnValue().Set(object);
    }

    DestroyImageInfo(imageInfo);
    DestroyExceptionInfo(&exception);
  }

  static void thumbnail(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    IMAGE_METHOD(ThumbnailImage, width, height)
  }

  static void sample(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    IMAGE_METHOD(SampleImage, width, height)
  }

  static void scale(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    IMAGE_METHOD(ScaleImage, width, height)
  }

  static void resize(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_INT_ARG(0, width)
    REQ_INT_ARG(1, height)
    OPT_INT_ARG(2, f, LanczosFilter)
    OPT_DOUBLE_ARG(3, blur, 1.0)
    FilterTypes filter = FilterTypes(f);
    IMAGE_METHOD(ResizeImage, width, height, filter, blur)
  }

  //http://www.graphicsmagick.org/api/transform.html#chopimage
  static void chop(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_RECT_ARG(0, chopInfo)
    IMAGE_METHOD(ChopImage, &chopInfo)
  }

  //http://www.graphicsmagick.org/api/transform.html#cropimage
  static void crop(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_RECT_ARG(0, cropInfo)
    IMAGE_METHOD(CropImage, &cropInfo)
  }

  //http://www.graphicsmagick.org/api/transform.html#extentimage
  static void extent(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_RECT_ARG(0, geometry)
    IMAGE_METHOD(ExtentImage, &geometry)
  }

  static void flip(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    IMAGE_METHOD(FlipImage)
  }

  static void flop(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    IMAGE_METHOD(FlopImage)
  }

  //http://www.graphicsmagick.org/api/shear.html#affinetransformimage
  static void affineTransform(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    AffineMatrix affineMatrix;
    IMAGE_METHOD(AffineTransformImage, &affineMatrix)
  }

  //http://www.graphicsmagick.org/api/shear.html#rotateimage
  static void rotate(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_DOUBLE_ARG(0, degrees)
    IMAGE_METHOD(RotateImage, degrees)
  }

  static void quality(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;
    REQ_DOUBLE_ARG(0, quality)
    MagickImage *image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    image->_quality = quality;
    info.GetReturnValue().SetUndefined();
  }

  static void format(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;
    REQ_STR_ARG(0, format)
    MagickImage *image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    image->_format = strdup(*format);
    info.GetReturnValue().SetUndefined();
  }

  //http://www.graphicsmagick.org/api/shear.html#shearimage
  static void shear(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    REQ_DOUBLE_ARG(0, x)
    REQ_DOUBLE_ARG(1, y)
    IMAGE_METHOD(ShearImage, x, y)
  }

  //http://www.graphicsmagick.org/api/enhance.html#contrastimage
  static void contrast(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //REQ_INT_ARG(0, s)
    //IMAGE_METHOD(ContrastImage, s)
    info.GetReturnValue().SetUndefined();
  }

  //http://www.graphicsmagick.org/api/enhance.html#equalizeimage
  static void equalize(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(EqualizeImage)
    info.GetReturnValue().SetUndefined();
  }

  static void gamma(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(GammaImage)
    info.GetReturnValue().SetUndefined();
  }

  static void level(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(LevelImage)
    info.GetReturnValue().SetUndefined();
  }

  static void levelChannel(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(LevelImageChannel)
    info.GetReturnValue().SetUndefined();
  }

  static void modulate(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(ModulateImage)
    info.GetReturnValue().SetUndefined();
  }

  static void negate(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(NegateImage)
    info.GetReturnValue().SetUndefined();
  }

  static void normalize(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //IMAGE_METHOD(NormalizeImage)
    info.GetReturnValue().SetUndefined();
  }

  //http://www.graphicsmagick.org/api/attribute.html#getimageattribute
  //http://www.graphicsmagick.org/api/attribute.html#setimageattribute
  static void attribute(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    //MagickImage *image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    //ExceptionInfo exception;
    info.GetReturnValue().SetUndefined();
  }

  //http://www.graphicsmagick.org/api/composite.html
  static void composite(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    Nan::HandleScope scope;
    Handle<Value> out;
    CompositeOperator compose;
    MagickImage *image = Nan::ObjectWrap::Unwrap<MagickImage>(info.This());
    REQ_IMG_ARG(0, i)
    REQ_INT_ARG(1, c)
    OPT_INT_ARG(2, x, 0)
    OPT_INT_ARG(3, y, 0)
    const Image* compositeImage = *i;
    compose = CompositeOperator(c);
    if (CompositeImage( *image, compose, compositeImage, x, y ) == MagickPass)
      info.GetReturnValue().Set(info.This());
    else
      return Nan::ThrowError("Unable to composite image!");
  }

};

Nan::Persistent<FunctionTemplate> MagickImage::constructorTemplate;

NAN_MODULE_INIT(init) {
  Nan::HandleScope scope;
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

  Nan::SetMethod(target, "image", MagickImage::create);

  MagickImage::Init(target);
}
NODE_MODULE(GraphicsMagick, init);
