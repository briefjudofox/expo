#include "EXGLContext.h"
#include "EXGLImageUtils.h"

#include <algorithm>

// Helpers for unpacking arguments from jsi::Value* jsArgv
// Arguments are assumed to be numbers (double) and implicitly casted
// to expected type.
//
// EXJS_UNPACK_ARGV(GLenum val1, GLint val2) will be resolved to
// GLenum val1 = jsArgv[0].asNumber() ; GLint val2 = jsArgv[0 +1].asNumber();

#define _EXJS_COMMA() ,
#define _EXJS_EMPTY()
#define _EXJS_LITERAL(X) X _EXJS_EMPTY

#define EXJS_MAP_EXT(OFFSET, JOINER, F, ...) \
  _EXJS_EVAL(_EXJS_MAP1(OFFSET, JOINER, F, __VA_ARGS__, (), 0))
#define EXJS_MAP(F, ...) _EXJS_EVAL(_EXJS_MAP1(0, _EXJS_EMPTY, F, __VA_ARGS__, (), 0))

#define _EXJS_EVAL0(...) __VA_ARGS__
#define _EXJS_EVAL1(...) _EXJS_EVAL0(_EXJS_EVAL0(__VA_ARGS__))
#define _EXJS_EVAL2(...) _EXJS_EVAL1(_EXJS_EVAL1(__VA_ARGS__))
#define _EXJS_EVAL(...) _EXJS_EVAL2(_EXJS_EVAL2(__VA_ARGS__))

#define _EXJS_MAP_END(...)
#define _EXJS_MAP_OUT
#define _EXJS_MAP_GET_END() 0, _EXJS_MAP_END
#define _EXJS_MAP_NEXT0(ITEM, NEXT, ...) NEXT _EXJS_MAP_OUT
#define _EXJS_MAP_NEXT1(JOINER, ITEM, NEXT) _EXJS_MAP_NEXT0(ITEM, JOINER() NEXT, 0)
#define _EXJS_MAP_NEXT(JOINER, ITEM, NEXT) _EXJS_MAP_NEXT1(JOINER, _EXJS_MAP_GET_END ITEM, NEXT)

#define _EXJS_MAP0(IDX, JOINER, F, NAME, PEEK, ...) \
  F(IDX, NAME) _EXJS_MAP_NEXT(JOINER, PEEK, _EXJS_MAP1)(IDX + 1, JOINER, F, PEEK, __VA_ARGS__)
#define _EXJS_MAP1(IDX, JOINER, F, NAME, PEEK, ...) \
  F(IDX, NAME) _EXJS_MAP_NEXT(JOINER, PEEK, _EXJS_MAP0)(IDX + 1, JOINER, F, PEEK, __VA_ARGS__)

#define EXJS_ARGC(...) \
  _EXJS_ARGC_SEQ(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define _EXJS_ARGC_SEQ(                                                            \
    x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, n, ...) \
  n

#define EXJS_UNPACK_ARGV(...) EXJS_UNPACK_ARGV_OFFSET(0, __VA_ARGS__)
#define EXJS_UNPACK_ARGV_OFFSET(OFFSET, ...) \
  EXJS_MAP_EXT(OFFSET, _EXJS_LITERAL(;), _EXJS_UNPACK_NUMBER, __VA_ARGS__)

#define _EXJS_UNPACK_NUMBER(INDEX, NAME) \
  NAME = jsArgv[INDEX].isBool() ? jsArgv[INDEX].getBool() : jsArgv[INDEX].asNumber()

// Wrapper functions for implementation of WebGLRenderingContext
// -----------------

// Wrapper raises an exception saying the function isn't implemented yet
#define ARGS(...) unpackArgs<__VA_ARGS__>(runtime, jsArgv, argc)
#define ARG(index, type) unpackArg<type>(runtime, jsArgv + index)

#define NATIVE_METHOD(name, ...)                 \
  jsi::Value EXGLContext::glNativeMethod_##name( \
      jsi::Runtime &runtime,                     \
      const jsi::Value &jsThis,                  \
      const jsi::Value *jsArgv,                  \
      size_t argc)

#define SIMPLE_NATIVE_METHOD(name, func, ...)                    \
  NATIVE_METHOD(name) {                                          \
    addToNextBatch(generateNativeMethod(runtime, func, jsArgv)); \
    return nullptr;                                              \
  }

#define UNIMPL_NATIVE_METHOD(name) \
  NATIVE_METHOD(name) {            \
    return unimplemented(#name);   \
  }

// This listing follows the order in
// https://developer.mozilla.org/en-US/docs/Web/API/WebGLRenderingContext
// https://developer.mozilla.org/en-US/docs/Web/API/WebGL2RenderingContext

// The WebGL context
// -----------------

NATIVE_METHOD(getContextAttributes) {
  jsi::Object jsResult(runtime);
  jsResult.setProperty(runtime, "alpha", true);
  jsResult.setProperty(runtime, "depth", true);
  jsResult.setProperty(runtime, "stencil", false);
  jsResult.setProperty(runtime, "antialias", false);
  jsResult.setProperty(runtime, "premultipliedAlpha", false);
  return jsResult;
}

NATIVE_METHOD(isContextLost) {
  return false;
}

// Viewing and clipping
// --------------------

SIMPLE_NATIVE_METHOD(scissor, glScissor);

SIMPLE_NATIVE_METHOD(viewport, glViewport)

// State information
// -----------------

SIMPLE_NATIVE_METHOD(activeTexture, glActiveTexture); // texture

SIMPLE_NATIVE_METHOD(blendColor, glBlendColor); // red, green, blue, alpha

SIMPLE_NATIVE_METHOD(blendEquation, glBlendEquation); // mode

SIMPLE_NATIVE_METHOD(blendEquationSeparate, glBlendEquationSeparate); // modeRGB, modeAlpha

SIMPLE_NATIVE_METHOD(blendFunc, glBlendFunc); // sfactor, dfactor

SIMPLE_NATIVE_METHOD(blendFuncSeparate, glBlendFuncSeparate); // srcRGB, dstRGB, srcAlpha, dstAlpha

SIMPLE_NATIVE_METHOD(clearColor, glClearColor); // red, green, blue, alpha

SIMPLE_NATIVE_METHOD(clearDepth, glClearDepthf); // depth

SIMPLE_NATIVE_METHOD(clearStencil, glClearStencil); // s

SIMPLE_NATIVE_METHOD(colorMask, glColorMask); // red, green, blue, alpha

SIMPLE_NATIVE_METHOD(cullFace, glCullFace); // mode

SIMPLE_NATIVE_METHOD(depthFunc, glDepthFunc); // func

SIMPLE_NATIVE_METHOD(depthMask, glDepthMask); // flag

SIMPLE_NATIVE_METHOD(depthRange, glDepthRangef); // zNear, zFar

SIMPLE_NATIVE_METHOD(disable, glDisable); // cap

SIMPLE_NATIVE_METHOD(enable, glEnable); // cap

SIMPLE_NATIVE_METHOD(frontFace, glFrontFace); // mode

NATIVE_METHOD(getParameter) {
  EXJS_UNPACK_ARGV(GLenum pname);

  switch (pname) {
      // Float32Array[0]
    case GL_COMPRESSED_TEXTURE_FORMATS:
      return jsi::TypedArray<jsi::TypedArrayKind::Float32Array>(runtime, {});

      // FLoat32Array[2]
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE: {
      std::vector<jsi::TypedArrayBase::ContentType<jsi::TypedArrayKind::Float32Array>> glResults(2);
      addBlockingToNextBatch([&] { glGetFloatv(pname, glResults.data()); });
      return jsi::TypedArray<jsi::TypedArrayKind::Float32Array>(runtime, glResults);
    }
      // FLoat32Array[4]
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE: {
      std::vector<jsi::TypedArrayBase::ContentType<jsi::TypedArrayKind::Float32Array>> glResults(4);
      addBlockingToNextBatch([&] { glGetFloatv(pname, glResults.data()); });
      return jsi::TypedArray<jsi::TypedArrayKind::Float32Array>(runtime, glResults);
    }
      // Int32Array[2]
    case GL_MAX_VIEWPORT_DIMS: {
      std::vector<jsi::TypedArrayBase::ContentType<jsi::TypedArrayKind::Int32Array>> glResults(2);
      addBlockingToNextBatch([&] { glGetIntegerv(pname, glResults.data()); });
      return jsi::TypedArray<jsi::TypedArrayKind::Int32Array>(runtime, glResults);
    }
      // Int32Array[4]
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT: {
      std::vector<jsi::TypedArrayBase::ContentType<jsi::TypedArrayKind::Int32Array>> glResults(4);
      addBlockingToNextBatch([&] { glGetIntegerv(pname, glResults.data()); });
      return jsi::TypedArray<jsi::TypedArrayKind::Int32Array>(runtime, glResults);
    }
      // boolean[4]
    case GL_COLOR_WRITEMASK: {
      GLint glResults[4];
      addBlockingToNextBatch([&] { glGetIntegerv(pname, glResults); });
      return jsi::Array::createWithElements(
          runtime,
          {jsi::Value(glResults[0]),
           jsi::Value(glResults[1]),
           jsi::Value(glResults[2]),
           jsi::Value(glResults[3])});
    }

      // boolean
    case GL_UNPACK_FLIP_Y_WEBGL:
      return unpackFLipY;
    case GL_UNPACK_PREMULTIPLY_ALPHA_WEBGL:
    case GL_UNPACK_COLORSPACE_CONVERSION_WEBGL:
      return false;
    case GL_RASTERIZER_DISCARD:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_TRANSFORM_FEEDBACK_ACTIVE:
    case GL_TRANSFORM_FEEDBACK_PAUSED: {
      GLint glResult;
      addBlockingToNextBatch([&] { glGetIntegerv(pname, &glResult); });
      return jsi::Value(glResult);
    }

      // string
    case GL_RENDERER:
    case GL_SHADING_LANGUAGE_VERSION:
    case GL_VENDOR:
    case GL_VERSION: {
      const GLubyte *glStr;
      addBlockingToNextBatch([&] { glStr = glGetString(pname); });
      return jsi::String::createFromUtf8(
          runtime, std::string(reinterpret_cast<const char *>(glStr)));
    }

      // float
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_MAX_TEXTURE_LOD_BIAS: {
      GLfloat glFloat;
      addBlockingToNextBatch([&] { glGetFloatv(pname, &glFloat); });
      return static_cast<double>(glFloat);
    }

      // UEXGLObjectId
    case GL_ARRAY_BUFFER_BINDING:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_CURRENT_PROGRAM: {
      GLint glInt;
      addBlockingToNextBatch([&] { glGetIntegerv(pname, &glInt); });
      for (const auto &pair : objects) {
        if (pair.second == glInt) {
          return static_cast<double>(pair.first);
        }
      }
      return nullptr;
    }

    // Unimplemented...
#define _GET_PARAMETER_UNIMPL(param) \
  case GL_##param:                   \
    throw std::runtime_error("EXGL: getParameter() doesn't support gl." #param " yet!");
      _GET_PARAMETER_UNIMPL(COPY_READ_BUFFER_BINDING)
      _GET_PARAMETER_UNIMPL(COPY_WRITE_BUFFER_BINDING)
      _GET_PARAMETER_UNIMPL(DRAW_FRAMEBUFFER_BINDING)
      _GET_PARAMETER_UNIMPL(READ_FRAMEBUFFER_BINDING)
      _GET_PARAMETER_UNIMPL(RENDERBUFFER)
      _GET_PARAMETER_UNIMPL(SAMPLER_BINDING)
      _GET_PARAMETER_UNIMPL(TEXTURE_BINDING_2D_ARRAY)
      _GET_PARAMETER_UNIMPL(TEXTURE_BINDING_2D)
      _GET_PARAMETER_UNIMPL(TEXTURE_BINDING_3D)
      _GET_PARAMETER_UNIMPL(TEXTURE_BINDING_CUBE_MAP)
      _GET_PARAMETER_UNIMPL(TRANSFORM_FEEDBACK_BINDING)
      _GET_PARAMETER_UNIMPL(TRANSFORM_FEEDBACK_BUFFER_BINDING)
      _GET_PARAMETER_UNIMPL(UNIFORM_BUFFER_BINDING)
      _GET_PARAMETER_UNIMPL(VERTEX_ARRAY_BINDING)
#undef _GET_PARAMETER_UNIMPL

      // int
    default: {
      GLint glInt;
      addBlockingToNextBatch([&] { glGetIntegerv(pname, &glInt); });
      return jsi::Value(glInt);
    }
  }
}

NATIVE_METHOD(getError, 0) {
  GLenum glResult;
  addBlockingToNextBatch([&] { glResult = glGetError(); });
  return static_cast<double>(glResult);
}

SIMPLE_NATIVE_METHOD(hint, glHint, target, mode)

NATIVE_METHOD(isEnabled, 1) {
  EXJS_UNPACK_ARGV(GLenum cap);
  GLboolean glResult;
  addBlockingToNextBatch([&] { glResult = glIsEnabled(cap); });
  return glResult == GL_TRUE;
}

SIMPLE_NATIVE_METHOD(lineWidth, glLineWidth, width)

NATIVE_METHOD(pixelStorei, 2) {
  EXJS_UNPACK_ARGV(GLenum pname);
  switch (pname) {
    case GL_UNPACK_FLIP_Y_WEBGL: {
      GLboolean param = jsValueToBool(runtime, jsArgv[1]);
      unpackFLipY = param;
      break;
    }
    default:
      jsConsoleLog(runtime, "EXGL: gl.pixelStorei() doesn't support this parameter yet!");
  }
  return nullptr;
}

SIMPLE_NATIVE_METHOD(polygonOffset, glPolygonOffset); // factor, units

SIMPLE_NATIVE_METHOD(sampleCoverage, glSampleCoverage); // value, invert

SIMPLE_NATIVE_METHOD(stencilFunc, glStencilFunc); // func, ref, mask

SIMPLE_NATIVE_METHOD(stencilFuncSeparate, glStencilFuncSeparate); // face, func, ref, mask

SIMPLE_NATIVE_METHOD(stencilMask, glStencilMask); // mask

SIMPLE_NATIVE_METHOD(stencilMaskSeparate, glStencilMaskSeparate, face, mask)

SIMPLE_NATIVE_METHOD(stencilOp, glStencilOp, fail, zfail, zpass)

SIMPLE_NATIVE_METHOD(stencilOpSeparate, glStencilOpSeparate, face, fail, zfail, zpass)

// Buffers
// -------

NATIVE_METHOD(bindBuffer, 2) {
  EXJS_UNPACK_ARGV(GLenum target);
  UEXGLObjectId fBuffer = jsArgv[1].isNull() ? 0 : jsArgv[1].asNumber();
  addToNextBatch([=] { glBindBuffer(target, fBuffer == 0 ? 0 : lookupObject(fBuffer)); });
  return nullptr;
}

NATIVE_METHOD(bufferData, 3) {
  GLenum target = jsArgv[0].asNumber();
  const jsi::Value &sizeOrData = jsArgv[1];
  GLenum usage = jsArgv[2].asNumber();

  if (sizeOrData.isNumber()) {
    GLsizeiptr length = sizeOrData.getNumber();
    addToNextBatch([=] { glBufferData(target, length, nullptr, usage); });
  } else if (sizeOrData.isNull()) {
    addToNextBatch([=] { glBufferData(target, 0, nullptr, usage); });
  } else if (sizeOrData.isObject()) {
    auto data = rawArrayBuffer(runtime, sizeOrData.getObject(runtime));
    addToNextBatch([=] { glBufferData(target, data.size(), data.data(), usage); });
  }
  return nullptr;
}

NATIVE_METHOD(bufferSubData, 3) {
  EXJS_UNPACK_ARGV(GLenum target, GLintptr offset);
  const jsi::Value &jsData = jsArgv[2];
  auto data = rawArrayBuffer(runtime, jsArgv[2].asObject(runtime));
  addToNextBatch([=] { glBufferSubData(target, offset, data.size(), data.data()); });
  return nullptr;
}

NATIVE_METHOD(createBuffer, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    return buffer;
  });
}

NATIVE_METHOD(deleteBuffer, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fBuffer);
  addToNextBatch([=] {
    GLuint buffer = lookupObject(fBuffer);
    glDeleteBuffers(1, &buffer);
  });
  return nullptr;
}

NATIVE_METHOD(getBufferParameter, 2) {
  EXJS_UNPACK_ARGV(GLenum target, GLenum pname);
  GLint glResult;
  addBlockingToNextBatch([&] { glGetBufferParameteriv(target, pname, &glResult); });
  return jsi::Value(glResult);
}

#define _WRAP_METHOD_IS_OBJECT_INTERNAL(type, requiresWebGL2)                \
  NATIVE_METHOD(is##type) {                                                  \
    EXJS_UNPACK_ARGV(UEXGLObjectId f);                                       \
    GLboolean glResult;                                                      \
    addBlockingToNextBatch([&] { glResult = glIs##type(lookupObject(f)); }); \
    return glResult == GL_TRUE;                                              \
  }

#define _WRAP_METHOD_IS_OBJECT(type) _WRAP_METHOD_IS_OBJECT_INTERNAL(type, false)
#define _WRAP_WEBGL2_METHOD_IS_OBJECT(type) _WRAP_METHOD_IS_OBJECT_INTERNAL(type, true)

_WRAP_METHOD_IS_OBJECT(Buffer)

// Buffers (WebGL2)

SIMPLE_NATIVE_METHOD(
    copyBufferSubData,
    glCopyBufferSubData,
    readTarget,
    writeTarget,
    readOffset,
    writeOffset,
    size)

// glGetBufferSubData is not available in OpenGL ES
UNIMPL_NATIVE_METHOD(getBufferSubData);

// Framebuffers
// ------------

NATIVE_METHOD(bindFramebuffer, 2) {
  EXJS_UNPACK_ARGV(GLenum target);
  if (jsArgv[1].isNull()) {
    addToNextBatch([=] { glBindFramebuffer(target, defaultFramebuffer); });
  } else {
    UEXGLObjectId fFramebuffer = jsArgv[1].asNumber();
    addToNextBatch([=] { glBindFramebuffer(target, lookupObject(fFramebuffer)); });
  }
  return nullptr;
}

NATIVE_METHOD(checkFramebufferStatus, 1) {
  EXJS_UNPACK_ARGV(GLenum target);
  GLenum glResult;
  addBlockingToNextBatch([&] { glResult = glCheckFramebufferStatus(target); });
  return static_cast<double>(glResult);
}

NATIVE_METHOD(createFramebuffer, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    return static_cast<double>(framebuffer);
  });
}

NATIVE_METHOD(deleteFramebuffer, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fFramebuffer);
  addToNextBatch([=] {
    GLuint framebuffer = lookupObject(fFramebuffer);
    glDeleteFramebuffers(1, &framebuffer);
  });
  return nullptr;
}

NATIVE_METHOD(framebufferRenderbuffer, 4) {
  EXJS_UNPACK_ARGV(
      GLenum target, GLenum attachment, GLenum renderbuffertarget, UEXGLObjectId fRenderbuffer);
  addToNextBatch([=] {
    GLuint renderbuffer = lookupObject(fRenderbuffer);
    glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
  });
  return nullptr;
}

NATIVE_METHOD(framebufferTexture2D, 5) {
  EXJS_UNPACK_ARGV(
      GLenum target, GLenum attachment, GLenum textarget, UEXGLObjectId fTexture, GLint level);
  addToNextBatch([=] {
    glFramebufferTexture2D(target, attachment, textarget, lookupObject(fTexture), level);
  });
  return nullptr;
}

UNIMPL_NATIVE_METHOD(getFramebufferAttachmentParameter)

_WRAP_METHOD_IS_OBJECT(Framebuffer)

NATIVE_METHOD(readPixels, 7) {
  EXJS_UNPACK_ARGV(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type);
  size_t byteLength = width * height * bytesPerPixel(type, format);
  auto pixels = std::vector<uint8_t>(byteLength);
  addBlockingToNextBatch([&] { glReadPixels(x, y, width, height, format, type, pixels.data()); });

  jsi::TypedArrayBase arr = jsArgv[6].asObject(runtime).asTypedArray(runtime);
  arr.getBuffer(runtime).update(runtime, pixels, arr.byteOffset(runtime));
  return nullptr;
}

// Framebuffers (WebGL2)
// ---------------------

SIMPLE_NATIVE_METHOD(
    blitFramebuffer,
    glBlitFramebuffer,
    srcX0,
    srcY0,
    srcX1,
    srcY1,
    dstX0,
    dstY0,
    dstX1,
    dstY1,
    mask,
    filter)

NATIVE_METHOD(framebufferTextureLayer, 5) {
  EXJS_UNPACK_ARGV(
      GLenum target, GLenum attachment, UEXGLObjectId texture, GLint level, GLint layer);
  addToNextBatch(
      [=] { glFramebufferTextureLayer(target, attachment, lookupObject(texture), level, layer); });
  return nullptr;
}

NATIVE_METHOD(invalidateFramebuffer, 2) {
  EXJS_UNPACK_ARGV(GLenum target);
  auto jsAttachments = jsArgv[1].asObject(runtime).asArray(runtime);
  std::vector<GLenum> attachments(jsAttachments.size(runtime));
  for (int i = 0; i < attachments.size(); i++) {
    attachments[i] = jsAttachments.getValueAtIndex(runtime, i).asNumber();
  }
  addToNextBatch([=] { glInvalidateFramebuffer(target, attachments.size(), attachments.data()); });
  return nullptr; // breaking change TypedArray -> Array (bug in previous implementation)
}

NATIVE_METHOD(invalidateSubFramebuffer, 6) {
  EXJS_UNPACK_ARGV(GLenum target);
  EXJS_UNPACK_ARGV_OFFSET(2, GLint x, GLint y, GLint width, GLint height);
  auto jsAttachments = jsArgv[1].asObject(runtime).asArray(runtime);
  std::vector<GLenum> attachments(jsAttachments.size(runtime));
  for (int i = 0; i < attachments.size(); i++) {
    attachments[i] = jsAttachments.getValueAtIndex(runtime, i).asNumber();
  }
  addToNextBatch([=] {
    glInvalidateSubFramebuffer(target, attachments.size(), attachments.data(), x, y, width, height);
  });
  return nullptr;
}

SIMPLE_NATIVE_METHOD(readBuffer, glReadBuffer, mode)

// Renderbuffers
// -------------

NATIVE_METHOD(bindRenderbuffer, 2) {
  EXJS_UNPACK_ARGV(GLenum target);
  UEXGLObjectId fRenderbuffer = jsArgv[1].isNull() ? 0 : jsArgv[1].asNumber();
  addToNextBatch([=] {
    GLuint renderbuffer = lookupObject(fRenderbuffer);
    glBindRenderbuffer(target, renderbuffer);
  });
  return nullptr;
}

NATIVE_METHOD(createRenderbuffer, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);
    return renderbuffer;
  });
}

NATIVE_METHOD(deleteRenderbuffer, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fRenderbuffer);
  addToNextBatch([=] {
    GLuint renderbuffer = lookupObject(fRenderbuffer);
    glDeleteRenderbuffers(1, &renderbuffer);
  });
  return nullptr;
}

UNIMPL_NATIVE_METHOD(getRenderbufferParameter)

_WRAP_METHOD_IS_OBJECT(Renderbuffer)

NATIVE_METHOD(renderbufferStorage, 4) {
  EXJS_UNPACK_ARGV(GLenum target, GLint internalformat, GLsizei width, GLsizei height);
  addToNextBatch([=] { glRenderbufferStorage(target, internalformat, width, height); });
  return nullptr;
}

// Renderbuffers (WebGL2)
// ----------------------

UNIMPL_NATIVE_METHOD(getInternalformatParameter)

UNIMPL_NATIVE_METHOD(renderbufferStorageMultisample)

// Textures
// --------

NATIVE_METHOD(bindTexture, 2) {
  EXJS_UNPACK_ARGV(GLenum target);
  if (jsArgv[1].isNull()) {
    addToNextBatch(std::bind(glBindTexture, target, 0));
  } else {
    UEXGLObjectId fTexture = jsArgv[1].asNumber();
    addToNextBatch([=] { glBindTexture(target, lookupObject(fTexture)); });
  }
  return nullptr;
}

UNIMPL_NATIVE_METHOD(compressedTexImage2D)

UNIMPL_NATIVE_METHOD(compressedTexSubImage2D)

SIMPLE_NATIVE_METHOD(
    copyTexImage2D,
    glCopyTexImage2D,
    target,
    level,
    internalformat,
    x,
    y,
    width,
    height,
    border)

SIMPLE_NATIVE_METHOD(
    copyTexSubImage2D,
    glCopyTexSubImage2D,
    target,
    level,
    xoffset,
    yoffset,
    x,
    y,
    width,
    height)

NATIVE_METHOD(createTexture, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint texture;
    glGenTextures(1, &texture);
    return texture;
  });
}

NATIVE_METHOD(deleteTexture, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fTexture);
  addToNextBatch([=] {
    GLuint texture = lookupObject(fTexture);
    glDeleteTextures(1, &texture);
  });
  return nullptr;
}

SIMPLE_NATIVE_METHOD(generateMipmap, glGenerateMipmap, target)

UNIMPL_NATIVE_METHOD(getTexParameter)

_WRAP_METHOD_IS_OBJECT(Texture)

NATIVE_METHOD(texImage2D, 6) {
  GLenum target;
  GLint level, internalformat;
  GLsizei width = 0, height = 0, border = 0;
  GLenum format, type;
  const jsi::Value *jsPixelsArg;

  if (argc == 9) {
    // 9-argument version
    EXJS_UNPACK_ARGV(target, level, internalformat, width, height, border, format, type);
    jsPixelsArg = &jsArgv[8];
  } else if (argc == 6) {
    // 6-argument version
    EXJS_UNPACK_ARGV(target, level, internalformat, format, type);
    jsPixelsArg = &jsArgv[5];
  } else {
    throw std::runtime_error("EXGL: Invalid number of arguments to gl.texImage2D()!");
  }
  const jsi::Value &jsPixels = *jsPixelsArg;

  if (jsPixels.isNull()) {
    addToNextBatch([=] {
      glTexImage2D(target, level, internalformat, width, height, border, format, type, nullptr);
    });
    return nullptr;
  }

  std::shared_ptr<uint8_t> data(nullptr);

  if (jsPixels.asObject(runtime).isArrayBuffer(runtime) ||
      jsPixels.asObject(runtime).isTypedArray(runtime)) {
    std::vector<uint8_t> vec = rawArrayBuffer(runtime, jsPixels.asObject(runtime));
    data = std::shared_ptr<uint8_t>(new uint8_t[vec.size()]);
    std::copy(vec.begin(), vec.end(), data.get());
  } else {
    data = loadImage(runtime, jsPixels, &width, &height, nullptr);
  }

  if (data) {
    if (unpackFLipY) {
      flipPixels((GLubyte *)data.get(), width * bytesPerPixel(type, format), height);
    }
    addToNextBatch([=] {
      glTexImage2D(target, level, internalformat, width, height, border, format, type, data.get());
    });
    return nullptr;
  }

  // Nothing worked...
  throw std::runtime_error("EXGL: Invalid pixel data argument for gl.texImage2D()!");
}

NATIVE_METHOD(texSubImage2D, 7) {
  GLenum target;
  GLint level, xoffset, yoffset;
  GLsizei width = 0, height = 0;
  GLenum format, type;
  const jsi::Value *jsPixelsArg;

  if (argc == 9) {
    // 9-argument version
    EXJS_UNPACK_ARGV(target, level, xoffset, yoffset, width, height, format, type);
    jsPixelsArg = &jsArgv[8];
  } else if (argc == 7) {
    // 7-argument version
    EXJS_UNPACK_ARGV(target, level, xoffset, yoffset, format, type);
    jsPixelsArg = &jsArgv[6];
  } else {
    throw std::runtime_error("EXGL: Invalid number of arguments to gl.texSubImage2D()!");
  }
  const jsi::Value &jsPixels = *jsPixelsArg;

  // Null?
  if (jsPixels.isNull()) {
    addToNextBatch([=] {
      void *nulled = calloc(width * height, bytesPerPixel(type, format));
      glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, nulled);
      free(nulled);
    });
    return nullptr;
  }

  std::shared_ptr<uint8_t> data(nullptr);

  if (jsPixels.asObject(runtime).isArrayBuffer(runtime) ||
      jsPixels.asObject(runtime).isTypedArray(runtime)) {
    std::vector<uint8_t> vec = rawArrayBuffer(runtime, jsPixels.asObject(runtime));
    data = std::shared_ptr<uint8_t>(new uint8_t[vec.size()]);
    std::copy(vec.begin(), vec.end(), data.get());
  } else {
    data = loadImage(runtime, jsPixels, &width, &height, nullptr);
  }

  if (data) {
    if (unpackFLipY) {
      flipPixels((GLubyte *)data.get(), width * bytesPerPixel(type, format), height);
    }
    addToNextBatch([=] {
      glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, data.get());
    });
    return nullptr;
  }

  // Nothing worked...
  throw std::runtime_error("EXGL: Invalid pixel data argument for gl.texSubImage2D()!");
}

SIMPLE_NATIVE_METHOD(texParameterf, glTexParameterf, target, pname, param)

SIMPLE_NATIVE_METHOD(texParameteri, glTexParameteri, target, pname, param)

// Textures (WebGL2)
// -----------------

SIMPLE_NATIVE_METHOD(texStorage2D, glTexStorage2D, target, levels, internalformat, width, height)

SIMPLE_NATIVE_METHOD(
    texStorage3D,
    glTexStorage3D,
    target,
    levels,
    internalformat,
    width,
    height,
    depth)

NATIVE_METHOD(texImage3D, 10) {
  GLenum target;
  GLint level, internalformat;
  GLsizei width, height, depth, border;
  GLenum format, type;

  EXJS_UNPACK_ARGV(target, level, internalformat, width, height, depth, border, format, type);
  const jsi::Value &jsPixels = jsArgv[9];

  if (jsPixels.isNull()) {
    addToNextBatch([=] {
      glTexImage3D(
          target, level, internalformat, width, height, depth, border, format, type, nullptr);
    });
    return nullptr;
  }

  std::shared_ptr<uint8_t> data(nullptr);

  if (jsPixels.asObject(runtime).isArrayBuffer(runtime) ||
      jsPixels.asObject(runtime).isTypedArray(runtime)) {
    std::vector<uint8_t> vec = rawArrayBuffer(runtime, jsPixels.asObject(runtime));
    data = std::shared_ptr<uint8_t>(new uint8_t[vec.size()]);
    std::copy(vec.begin(), vec.end(), data.get());
  } else {
    data = loadImage(runtime, jsPixels, &width, &height, nullptr);
  }

  if (data) {
    if (unpackFLipY) {
      GLubyte *texels = (GLubyte *)data.get();
      GLubyte *texelLayer = texels;
      for (int z = 0; z < depth; z++) {
        flipPixels(texelLayer, width * bytesPerPixel(type, format), height);
        texelLayer += bytesPerPixel(type, format) * width * height;
      }
    }
    addToNextBatch([=] {
      glTexImage3D(
          target, level, internalformat, width, height, depth, border, format, type, data.get());
    });
    return nullptr;
  }

  // Nothing worked...
  throw std::runtime_error("EXGL: Invalid pixel data argument for gl.texImage3D()!");
}

NATIVE_METHOD(texSubImage3D, 11) {
  GLenum target;
  GLint level, xoffset, yoffset, zoffset;
  GLsizei width, height, depth;
  GLenum format, type;

  EXJS_UNPACK_ARGV(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type);
  const jsi::Value &jsPixels = jsArgv[10];

  if (jsPixels.isNull()) {
    addToNextBatch([=] {
      void *nulled = calloc(width * height, bytesPerPixel(type, format));
      glTexSubImage3D(
          target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, nulled);
      free(nulled);
    });
    return nullptr;
  }

  std::shared_ptr<uint8_t> data(nullptr);

  if (jsPixels.asObject(runtime).isArrayBuffer(runtime) ||
      jsPixels.asObject(runtime).isTypedArray(runtime)) {
    std::vector<uint8_t> vec = rawArrayBuffer(runtime, jsPixels.asObject(runtime));
    data = std::shared_ptr<uint8_t>(new uint8_t[vec.size()]);
    std::copy(vec.begin(), vec.end(), data.get());
  } else {
    data = loadImage(runtime, jsPixels, &width, &height, nullptr);
  }

  if (data) {
    if (unpackFLipY) {
      GLubyte *texels = (GLubyte *)data.get();
      GLubyte *texelLayer = texels;
      for (int z = 0; z < depth; z++) {
        flipPixels(texelLayer, width * bytesPerPixel(type, format), height);
        texelLayer += bytesPerPixel(type, format) * width * height;
      }
    }
    addToNextBatch([=] {
      glTexSubImage3D(
          target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data.get());
    });
    return nullptr;
  }

  // Nothing worked...
  throw std::runtime_error("EXGL: Invalid pixel data argument for gl.texSubImage3D()!");
}

SIMPLE_NATIVE_METHOD(
    copyTexSubImage3D,
    glCopyTexSubImage3D,
    target,
    level,
    xoffset,
    yoffset,
    zoffset,
    x,
    y,
    width,
    height)

UNIMPL_NATIVE_METHOD(compressedTexImage3D)

UNIMPL_NATIVE_METHOD(compressedTexSubImage3D)

// Programs and shaders
// --------------------

NATIVE_METHOD(attachShader, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram, UEXGLObjectId fShader);
  addToNextBatch([=] { glAttachShader(lookupObject(fProgram), lookupObject(fShader)); });
  return nullptr;
}

NATIVE_METHOD(bindAttribLocation, 3) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram, GLuint index);
  auto name = jsArgv[2].asString(runtime).utf8(runtime);
  addToNextBatch([=] { glBindAttribLocation(lookupObject(fProgram), index, name.c_str()); });
  return nullptr;
}

NATIVE_METHOD(compileShader, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fShader);
  addToNextBatch([=] { glCompileShader(lookupObject(fShader)); });
  return nullptr;
}

NATIVE_METHOD(createProgram, 0) {
  return addFutureToNextBatch(runtime, &glCreateProgram);
}

NATIVE_METHOD(createShader, 1) {
  EXJS_UNPACK_ARGV(GLenum type);
  if (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER) {
    return addFutureToNextBatch(runtime, std::bind(glCreateShader, type));
  } else {
    return jsi::Value::null();
  }
}

NATIVE_METHOD(deleteProgram, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);
  addToNextBatch([=] { glDeleteProgram(lookupObject(fProgram)); });
  return nullptr;
}

NATIVE_METHOD(deleteShader, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fShader);
  addToNextBatch([=] { glDeleteShader(lookupObject(fShader)); });
  return nullptr;
}

NATIVE_METHOD(detachShader, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram, UEXGLObjectId fShader);
  addToNextBatch([=] { glDetachShader(lookupObject(fProgram), lookupObject(fShader)); });
  return nullptr;
}

NATIVE_METHOD(getAttachedShaders, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);

  GLint count;
  std::vector<GLuint> glResults;
  addBlockingToNextBatch([&] {
    GLuint program = lookupObject(fProgram);
    glGetProgramiv(program, GL_ATTACHED_SHADERS, &count);
    glResults.resize(count);
    glGetAttachedShaders(program, count, nullptr, glResults.data());
  });

  jsi::Array jsResults(runtime, count);
  for (auto i = 0; i < count; ++i) {
    UEXGLObjectId exglObjId = 0;
    for (const auto &pair : objects) {
      if (pair.second == glResults[i]) {
        exglObjId = pair.first;
      }
    }
    if (exglObjId == 0) {
      throw std::runtime_error(
          "EXGL: Internal error: couldn't find UEXGLObjectId "
          "associated with shader in getAttachedShaders()!");
    }
    jsResults.setValueAtIndex(runtime, i, static_cast<double>(exglObjId));
  }
  return jsResults;
}

NATIVE_METHOD(getProgramParameter, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram, GLenum pname);
  GLint glResult;
  addBlockingToNextBatch([&] { glGetProgramiv(lookupObject(fProgram), pname, &glResult); });
  if (pname == GL_DELETE_STATUS || pname == GL_LINK_STATUS || pname == GL_VALIDATE_STATUS) {
    return glResult == GL_TRUE;
  } else {
    return glResult;
  }
}

NATIVE_METHOD(getShaderParameter, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fShader, GLenum pname);
  GLint glResult;
  addBlockingToNextBatch([&] { glGetShaderiv(lookupObject(fShader), pname, &glResult); });
  if (pname == GL_DELETE_STATUS || pname == GL_COMPILE_STATUS) {
    return glResult == GL_TRUE;
  } else {
    return glResult;
  }
}

NATIVE_METHOD(getShaderPrecisionFormat, 2) {
  EXJS_UNPACK_ARGV(GLenum shaderType, GLenum precisionType);

  GLint range[2], precision;
  addBlockingToNextBatch(
      [&] { glGetShaderPrecisionFormat(shaderType, precisionType, range, &precision); });

  jsi::Object jsResult(runtime);
  jsResult.setProperty(runtime, "rangeMin", jsi::Value(range[0]));
  jsResult.setProperty(runtime, "rangeMax", jsi::Value(range[1]));
  jsResult.setProperty(runtime, "precision", jsi::Value(precision));
  return jsResult;
}

NATIVE_METHOD(getProgramInfoLog, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fObj);
  GLint length;
  std::string str;
  addBlockingToNextBatch([&] {
    GLuint obj = lookupObject(fObj);
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &length);
    str.resize(length);
    glGetProgramInfoLog(obj, length, nullptr, &str[0]);
  });
  return jsi::String::createFromUtf8(runtime, str);
}

NATIVE_METHOD(getShaderInfoLog, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fObj);
  GLint length;
  std::string str;
  addBlockingToNextBatch([&] {
    GLuint obj = lookupObject(fObj);
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length);
    str.resize(length);
    glGetShaderInfoLog(obj, length, nullptr, &str[0]);
  });
  return jsi::String::createFromUtf8(runtime, str);
}

NATIVE_METHOD(getShaderSource, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fObj);
  GLint length;
  std::string str;
  addBlockingToNextBatch([&] {
    GLuint obj = lookupObject(fObj);
    glGetShaderiv(obj, GL_SHADER_SOURCE_LENGTH, &length);
    str.resize(length);
    glGetShaderSource(obj, length, nullptr, &str[0]);
  });
  return jsi::String::createFromUtf8(runtime, str);
}

_WRAP_METHOD_IS_OBJECT(Program)

_WRAP_METHOD_IS_OBJECT(Shader)

NATIVE_METHOD(linkProgram, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);
  addToNextBatch([=] { glLinkProgram(lookupObject(fProgram)); });
  return nullptr;
}

NATIVE_METHOD(shaderSource, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fShader);
  std::string str = jsArgv[1].asString(runtime).utf8(runtime);
  addToNextBatch([=] {
    const char *cstr = str.c_str();
    glShaderSource(lookupObject(fShader), 1, &cstr, nullptr);
  });
  return nullptr;
}

NATIVE_METHOD(useProgram, 1) {
  if (jsArgv[0].isNull()) {
    addToNextBatch(std::bind(glUseProgram, 0));
  } else {
    EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);
    addToNextBatch([=] { glUseProgram(lookupObject(fProgram)); });
  }
  return nullptr;
}

NATIVE_METHOD(validateProgram, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);
  addToNextBatch([=] { glValidateProgram(lookupObject(fProgram)); });
  return nullptr;
}

// Programs and shaders (WebGL2)

NATIVE_METHOD(getFragDataLocation, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId program);
  std::string name = jsArgv[1].asString(runtime).utf8(runtime);
  GLint location;
  addBlockingToNextBatch(
      [&] { location = glGetFragDataLocation(lookupObject(program), name.c_str()); });
  return location == -1 ? jsi::Value::null() : jsi::Value(location);
}

// Uniforms and attributes
// -----------------------

SIMPLE_NATIVE_METHOD(disableVertexAttribArray, glDisableVertexAttribArray, index)

SIMPLE_NATIVE_METHOD(enableVertexAttribArray, glEnableVertexAttribArray, index)

template <typename F>
inline jsi::Value EXGLContext::getActiveInfo(
    jsi::Runtime &runtime,
    const jsi::Value *jsArgv,
    GLenum lengthParam,
    F &&glFunc) {
  if (jsArgv[0].isNull()) {
    return nullptr;
  }

  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram, GLuint index);

  GLsizei length;
  GLint size;
  GLenum type;
  std::string name;
  GLint maxNameLength;
  addBlockingToNextBatch([&] {
    GLuint program = lookupObject(fProgram);
    glGetProgramiv(program, lengthParam, &maxNameLength);
    name.resize(maxNameLength);
    glFunc(program, index, maxNameLength, &length, &size, &type, &name[0]);
  });

  if (strlen(name.c_str()) == 0) { // name.length() may be larger
    return jsi::Value::null();
  }

  jsi::Object jsResult(runtime);
  jsResult.setProperty(runtime, "name", jsi::String::createFromAscii(runtime, name.c_str()));
  jsResult.setProperty(runtime, "size", size);
  jsResult.setProperty(runtime, "type", static_cast<double>(type));
  return jsResult;
}

NATIVE_METHOD(getActiveAttrib, 2) {
  return getActiveInfo(runtime, jsArgv, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, glGetActiveAttrib);
}

NATIVE_METHOD(getActiveUniform, 2) {
  return getActiveInfo(runtime, jsArgv, GL_ACTIVE_UNIFORM_MAX_LENGTH, glGetActiveUniform);
}

NATIVE_METHOD(getAttribLocation, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);
  std::string name = jsArgv[1].asString(runtime).utf8(runtime);
  GLint location;
  addBlockingToNextBatch(
      [&] { location = glGetAttribLocation(lookupObject(fProgram), name.c_str()); });
  return jsi::Value(location);
}

UNIMPL_NATIVE_METHOD(getUniform)

NATIVE_METHOD(getUniformLocation, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram);
  std::string name = jsArgv[1].asString(runtime).utf8(runtime);
  GLint location;
  addBlockingToNextBatch(
      [&] { location = glGetUniformLocation(lookupObject(fProgram), name.c_str()); });
  return location == -1 ? jsi::Value::null() : location;
}

UNIMPL_NATIVE_METHOD(getVertexAttrib)

UNIMPL_NATIVE_METHOD(getVertexAttribOffset)

SIMPLE_NATIVE_METHOD(uniform1f, glUniform1f, uniform, x)
SIMPLE_NATIVE_METHOD(uniform2f, glUniform2f, uniform, x, y)
SIMPLE_NATIVE_METHOD(uniform3f, glUniform3f, uniform, x, y, z)
SIMPLE_NATIVE_METHOD(uniform4f, glUniform4f, uniform, x, y, z, w)
SIMPLE_NATIVE_METHOD(uniform1i, glUniform1i, uniform, x)
SIMPLE_NATIVE_METHOD(uniform2i, glUniform2i, uniform, x, y)
SIMPLE_NATIVE_METHOD(uniform3i, glUniform3i, uniform, x, y, z)
SIMPLE_NATIVE_METHOD(uniform4i, glUniform4i, uniform, x, y, z, w)

#define _WRAP_METHOD_UNIFORM_V(suffix, dim, Type, ArrayType)                                    \
  NATIVE_METHOD(uniform##suffix, 2) {                                                           \
    GLuint uniform = jsArgv[0].asNumber();                                                      \
    std::vector<Type> data =                                                                    \
        jsArgv[1].asObject(runtime).asTypedArray(runtime).as<ArrayType>(runtime).data(runtime); \
    addToNextBatch([=] { glUniform##suffix(uniform, data.size() / dim, data.data()); });        \
    return nullptr;                                                                             \
  }

_WRAP_METHOD_UNIFORM_V(1fv, 1, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_UNIFORM_V(2fv, 2, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_UNIFORM_V(3fv, 3, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_UNIFORM_V(4fv, 4, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_UNIFORM_V(1iv, 1, GLint, jsi::TypedArrayKind::Int32Array)
_WRAP_METHOD_UNIFORM_V(2iv, 2, GLint, jsi::TypedArrayKind::Int32Array)
_WRAP_METHOD_UNIFORM_V(3iv, 3, GLint, jsi::TypedArrayKind::Int32Array)
_WRAP_METHOD_UNIFORM_V(4iv, 4, GLint, jsi::TypedArrayKind::Int32Array)

#define _WRAP_METHOD_UNIFORM_MATRIX(suffix, dim)                                               \
  NATIVE_METHOD(uniformMatrix##suffix, 3) {                                                    \
    GLuint uniform = jsArgv[0].asNumber();                                                     \
    GLboolean transpose = jsValueToBool(runtime, jsArgv[1]);                                   \
    std::vector<GLfloat> data = jsArgv[2]                                                      \
                                    .asObject(runtime)                                         \
                                    .asTypedArray(runtime)                                     \
                                    .as<jsi::TypedArrayKind::Float32Array>(runtime)            \
                                    .data(runtime);                                            \
    addToNextBatch(                                                                            \
        [=] { glUniformMatrix##suffix(uniform, data.size() / dim, transpose, data.data()); }); \
    return nullptr;                                                                            \
  }

_WRAP_METHOD_UNIFORM_MATRIX(2fv, 4)
_WRAP_METHOD_UNIFORM_MATRIX(3fv, 9)
_WRAP_METHOD_UNIFORM_MATRIX(4fv, 16)

#define _WRAP_METHOD_VERTEX_ATTRIB_V(suffix, Type, ArrayType)                                   \
  NATIVE_METHOD(vertexAttrib##suffix, 2) {                                                      \
    GLuint index = jsArgv[0].asNumber();                                                        \
    auto data =                                                                                 \
        jsArgv[1].asObject(runtime).asTypedArray(runtime).as<ArrayType>(runtime).data(runtime); \
    addToNextBatch([=] { glVertexAttrib##suffix(index, data.data()); });                        \
    return nullptr;                                                                             \
  }
_WRAP_METHOD_VERTEX_ATTRIB_V(1fv, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_VERTEX_ATTRIB_V(2fv, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_VERTEX_ATTRIB_V(3fv, GLfloat, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_VERTEX_ATTRIB_V(4fv, GLfloat, jsi::TypedArrayKind::Float32Array)

SIMPLE_NATIVE_METHOD(vertexAttrib1f, glVertexAttrib1f, index, x)
SIMPLE_NATIVE_METHOD(vertexAttrib2f, glVertexAttrib2f, index, x, y)
SIMPLE_NATIVE_METHOD(vertexAttrib3f, glVertexAttrib3f, index, x, y, z)
SIMPLE_NATIVE_METHOD(vertexAttrib4f, glVertexAttrib4f, index, x, y, z, w)

NATIVE_METHOD(vertexAttribPointer, 6) {
  EXJS_UNPACK_ARGV(GLuint index, GLuint itemSize, GLenum type);
  GLboolean normalized = jsValueToBool(runtime, jsArgv[3]);
  EXJS_UNPACK_ARGV_OFFSET(4, GLsizei stride, GLint offset);
  addToNextBatch(std::bind(
      glVertexAttribPointer,
      index,
      itemSize,
      type,
      normalized,
      stride,
      reinterpret_cast<const void *>(offset)));
  return nullptr;
}

// Uniforms and attributes (WebGL2)
// --------------------------------

SIMPLE_NATIVE_METHOD(uniform1ui, glUniform1ui, location, x)
SIMPLE_NATIVE_METHOD(uniform2ui, glUniform2ui, location, x, y)
SIMPLE_NATIVE_METHOD(uniform3ui, glUniform3ui, location, x, y, z)
SIMPLE_NATIVE_METHOD(uniform4ui, glUniform4ui, location, x, y, z, w)

_WRAP_METHOD_UNIFORM_V(1uiv, 1, GLuint, jsi::TypedArrayKind::Uint32Array)
_WRAP_METHOD_UNIFORM_V(2uiv, 2, GLuint, jsi::TypedArrayKind::Uint32Array)
_WRAP_METHOD_UNIFORM_V(3uiv, 3, GLuint, jsi::TypedArrayKind::Uint32Array)
_WRAP_METHOD_UNIFORM_V(4uiv, 4, GLuint, jsi::TypedArrayKind::Uint32Array)

_WRAP_METHOD_UNIFORM_MATRIX(3x2fv, 6)
_WRAP_METHOD_UNIFORM_MATRIX(4x2fv, 8)
_WRAP_METHOD_UNIFORM_MATRIX(2x3fv, 6)
_WRAP_METHOD_UNIFORM_MATRIX(4x3fv, 12)
_WRAP_METHOD_UNIFORM_MATRIX(2x4fv, 8)
_WRAP_METHOD_UNIFORM_MATRIX(3x4fv, 12)

SIMPLE_NATIVE_METHOD(vertexAttribI4i, glVertexAttribI4i, index, x, y, z, w)
SIMPLE_NATIVE_METHOD(vertexAttribI4ui, glVertexAttribI4ui, index, x, y, z, w)

_WRAP_METHOD_VERTEX_ATTRIB_V(I4iv, GLint, jsi::TypedArrayKind::Int32Array)
_WRAP_METHOD_VERTEX_ATTRIB_V(I4uiv, GLuint, jsi::TypedArrayKind::Uint32Array)

NATIVE_METHOD(vertexAttribIPointer, 5) {
  EXJS_UNPACK_ARGV(GLuint index, GLuint size, GLenum type, GLsizei stride, GLint offset);
  addToNextBatch(std::bind(
      glVertexAttribIPointer, index, size, type, stride, reinterpret_cast<const void *>(offset)));
  return nullptr;
}

#undef _WRAP_METHOD_UNIFORM_V
#undef _WRAP_METHOD_UNIFORM_MATRIX
#undef _WRAP_METHOD_VERTEX_ATTRIB_V

// Drawing buffers
// ---------------

SIMPLE_NATIVE_METHOD(clear, glClear, mask)

SIMPLE_NATIVE_METHOD(drawArrays, glDrawArrays, mode, first, count)

NATIVE_METHOD(drawElements, 4) {
  EXJS_UNPACK_ARGV(GLenum mode, GLsizei count, GLenum type, GLint offset);
  addToNextBatch(
      std::bind(glDrawElements, mode, count, type, reinterpret_cast<const void *>(offset)));
  return nullptr;
}

NATIVE_METHOD(finish, 0) {
  addToNextBatch(glFinish);
  return nullptr;
}

NATIVE_METHOD(flush, 0) {
  addToNextBatch(glFlush);
  return nullptr;
}

// Drawing buffers (WebGL2)
// ------------------------

SIMPLE_NATIVE_METHOD(vertexAttribDivisor, glVertexAttribDivisor, index, divisor)

SIMPLE_NATIVE_METHOD(drawArraysInstanced, glDrawArraysInstanced, mode, first, count, instancecount)

NATIVE_METHOD(drawElementsInstanced, 5) {
  EXJS_UNPACK_ARGV(GLenum mode, GLsizei count, GLenum type, GLint offset, GLsizei instanceCount);
  addToNextBatch([=] {
    glDrawElementsInstanced(
        mode, count, type, reinterpret_cast<const void *>(offset), instanceCount);
  });
  return nullptr;
}

NATIVE_METHOD(drawRangeElements, 6) {
  EXJS_UNPACK_ARGV(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, GLint offset);
  addToNextBatch([=] {
    glDrawRangeElements(mode, start, end, count, type, reinterpret_cast<const void *>(offset));
  });
  return nullptr;
}

NATIVE_METHOD(drawBuffers, 1) {
  auto data = jsArrayToVector<GLenum>(runtime, jsArgv[0].asObject(runtime).asArray(runtime));
  addToNextBatch([=] { glDrawBuffers(data.size(), data.data()); });
  return nullptr;
}

#define _WRAP_METHOD_CLEAR_BUFFER(suffix, ArrayType)                                            \
  NATIVE_METHOD(clearBuffer##suffix, 4) {                                                       \
    EXJS_UNPACK_ARGV(GLenum buffer, GLint drawbuffer);                                          \
    auto values =                                                                               \
        jsArgv[2].asObject(runtime).asTypedArray(runtime).as<ArrayType>(runtime).data(runtime); \
    addToNextBatch([=] { glClearBuffer##suffix(buffer, drawbuffer, values.data()); });          \
    return nullptr;                                                                             \
  }

_WRAP_METHOD_CLEAR_BUFFER(fv, jsi::TypedArrayKind::Float32Array)
_WRAP_METHOD_CLEAR_BUFFER(iv, jsi::TypedArrayKind::Int32Array)
_WRAP_METHOD_CLEAR_BUFFER(uiv, jsi::TypedArrayKind::Uint32Array)

#undef _WRAP_METHOD_CLEAR_BUFFER

SIMPLE_NATIVE_METHOD(clearBufferfi, glClearBufferfi, buffer, drawbuffer, depth, stencil)

// Query objects (WebGL2)
// ----------------------

NATIVE_METHOD(createQuery, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint query;
    glGenQueries(1, &query);
    return query;
  });
}

NATIVE_METHOD(deleteQuery, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fQuery);
  addToNextBatch([=] {
    GLuint query = lookupObject(fQuery);
    glDeleteQueries(1, &query);
  });
  return nullptr;
}

_WRAP_WEBGL2_METHOD_IS_OBJECT(Query)

NATIVE_METHOD(beginQuery, 2) {
  EXJS_UNPACK_ARGV(GLenum target, UEXGLObjectId query);
  addToNextBatch([=] { glBeginQuery(target, lookupObject(query)); });
  return nullptr;
}

SIMPLE_NATIVE_METHOD(endQuery, glEndQuery, target)

NATIVE_METHOD(getQuery, 2) {
  EXJS_UNPACK_ARGV(GLenum target, GLenum pname);
  GLint params;
  addBlockingToNextBatch([&] { glGetQueryiv(target, pname, &params); });
  return params == 0 ? jsi::Value::null() : jsi::Value(params);
}

NATIVE_METHOD(getQueryParameter, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId query, GLenum pname);
  GLuint params;
  addBlockingToNextBatch([&] { glGetQueryObjectuiv(lookupObject(query), pname, &params); });
  return params == 0 ? jsi::Value::null() : static_cast<double>(params);
}

// Samplers (WebGL2)
// -----------------

NATIVE_METHOD(createSampler, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint sampler;
    glGenSamplers(1, &sampler);
    return sampler;
  });
}

NATIVE_METHOD(deleteSampler, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fSampler);
  addToNextBatch([=] {
    GLuint sampler = lookupObject(fSampler);
    glDeleteSamplers(1, &sampler);
  });
  return nullptr;
}

NATIVE_METHOD(bindSampler, 2) {
  EXJS_UNPACK_ARGV(GLuint unit, UEXGLObjectId sampler);
  addToNextBatch([=] { glBindSampler(unit, lookupObject(sampler)); });
  return nullptr;
}

_WRAP_WEBGL2_METHOD_IS_OBJECT(Sampler)

NATIVE_METHOD(samplerParameteri, 3) {
  EXJS_UNPACK_ARGV(UEXGLObjectId sampler, GLenum pname, GLint param);
  addToNextBatch([=] { glSamplerParameteri(lookupObject(sampler), pname, param); });
  return nullptr;
}

NATIVE_METHOD(samplerParameterf, 3) {
  EXJS_UNPACK_ARGV(UEXGLObjectId sampler, GLenum pname, GLfloat param);
  addToNextBatch([=] { glSamplerParameterf(lookupObject(sampler), pname, param); });
  return nullptr;
}

NATIVE_METHOD(getSamplerParameter, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fSampler, GLenum pname);
  bool isFloatParam = pname == GL_TEXTURE_MAX_LOD || pname == GL_TEXTURE_MIN_LOD;
  GLfloat paramf;
  GLint parami;

  addBlockingToNextBatch([&] {
    GLuint sampler = lookupObject(fSampler);

    if (isFloatParam) {
      glGetSamplerParameterfv(sampler, pname, &paramf);
    } else {
      glGetSamplerParameteriv(sampler, pname, &parami);
    }
  });
  return isFloatParam ? static_cast<double>(paramf) : static_cast<double>(parami);
}

// Sync objects (WebGL2)
// ---------------------

UNIMPL_NATIVE_METHOD(fenceSync)

UNIMPL_NATIVE_METHOD(isSync)

UNIMPL_NATIVE_METHOD(deleteSync)

UNIMPL_NATIVE_METHOD(clientWaitSync)

UNIMPL_NATIVE_METHOD(waitSync)

UNIMPL_NATIVE_METHOD(getSyncParameter)

// Transform feedback (WebGL2)
// ---------------------------

NATIVE_METHOD(createTransformFeedback, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint transformFeedback;
    glGenTransformFeedbacks(1, &transformFeedback);
    return transformFeedback;
  });
}

NATIVE_METHOD(deleteTransformFeedback, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fTransformFeedback);
  addToNextBatch([=] {
    GLuint transformFeedback = lookupObject(fTransformFeedback);
    glDeleteTransformFeedbacks(1, &transformFeedback);
  });
  return nullptr;
}

_WRAP_WEBGL2_METHOD_IS_OBJECT(TransformFeedback)

NATIVE_METHOD(bindTransformFeedback, 1) {
  EXJS_UNPACK_ARGV(GLenum target, UEXGLObjectId transformFeedback);
  addToNextBatch([=] { glBindTransformFeedback(target, lookupObject(transformFeedback)); });
  return nullptr;
}

SIMPLE_NATIVE_METHOD(beginTransformFeedback, glBeginTransformFeedback, primitiveMode)

NATIVE_METHOD(endTransformFeedback, 0) {
  addToNextBatch([=] { glEndTransformFeedback(); });
  return nullptr;
}

NATIVE_METHOD(transformFeedbackVaryings, 3) {
  UEXGLObjectId program = jsArgv[0].asNumber();
  GLenum bufferMode = jsArgv[2].asNumber();
  std::vector<std::string> varyings =
      jsArrayToVector<std::string>(runtime, jsArgv[1].asObject(runtime).asArray(runtime));

  addToNextBatch([=] {
    std::vector<const char *> varyingsRaw(varyings.size());
    std::transform(
        varyings.begin(), varyings.end(), varyingsRaw.begin(), [](const std::string &str) {
          return str.c_str();
        });

    glTransformFeedbackVaryings(
        lookupObject(program), varyingsRaw.size(), varyingsRaw.data(), bufferMode);
  });
  return nullptr;
}

NATIVE_METHOD(getTransformFeedbackVarying, 2) {
  return getActiveInfo(
      runtime, jsArgv, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, glGetTransformFeedbackVarying);
}

NATIVE_METHOD(pauseTransformFeedback, 0) {
  addToNextBatch([=] { glPauseTransformFeedback(); });
  return nullptr;
}

NATIVE_METHOD(resumeTransformFeedback, 0) {
  addToNextBatch([=] { glResumeTransformFeedback(); });
  return nullptr;
}

// Uniform buffer objects (WebGL2)
// -------------------------------

NATIVE_METHOD(bindBufferBase, 3) {
  EXJS_UNPACK_ARGV(GLenum target, GLuint index, UEXGLObjectId buffer);
  addToNextBatch([=] { glBindBufferBase(target, index, lookupObject(buffer)); });
  return nullptr;
}

NATIVE_METHOD(bindBufferRange, 5) {
  EXJS_UNPACK_ARGV(GLenum target, GLuint index, UEXGLObjectId buffer, GLint offset, GLsizei size);
  addToNextBatch([=] { glBindBufferRange(target, index, lookupObject(buffer), offset, size); });
  return nullptr;
}

NATIVE_METHOD(getUniformIndices, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId program);
  std::vector<std::string> uniformNames =
      jsArrayToVector<std::string>(runtime, jsArgv[1].asObject(runtime).asArray(runtime));

  std::vector<const char *> uniformNamesRaw(uniformNames.size());
  std::transform(
      uniformNames.begin(),
      uniformNames.end(),
      uniformNamesRaw.begin(),
      [](const std::string &str) { return str.c_str(); });

  GLuint indices[uniformNames.size()];
  addBlockingToNextBatch([&] {
    glGetUniformIndices(
        lookupObject(program), uniformNames.size(), uniformNamesRaw.data(), indices);
  });
  return jsi::TypedArray<jsi::TypedArrayKind::Uint32Array>(
      runtime, std::vector<GLuint>(indices, indices + uniformNames.size()));
}

NATIVE_METHOD(getActiveUniforms, 3) {
  EXJS_UNPACK_ARGV(UEXGLObjectId program);
  EXJS_UNPACK_ARGV_OFFSET(2, GLenum pname);
  auto uniformIndices =
      jsArrayToVector<GLuint>(runtime, jsArgv[1].asObject(runtime).asArray(runtime));
  GLint params[uniformIndices.size()];

  addBlockingToNextBatch([&] {
    glGetActiveUniformsiv(
        lookupObject(program),
        (GLsizei)uniformIndices.size(),
        uniformIndices.data(),
        pname,
        params);
  });
  return jsi::TypedArray<jsi::TypedArrayKind::Int32Array>(
      runtime, std::vector<GLint>(params, params + uniformIndices.size()));
}

NATIVE_METHOD(getUniformBlockIndex, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId program);
  std::string uniformBlockName = jsArgv[1].asString(runtime).utf8(runtime);
  GLuint blockIndex;

  addBlockingToNextBatch([&] {
    blockIndex = glGetUniformBlockIndex(lookupObject(program), uniformBlockName.c_str());
  });
  return static_cast<double>(blockIndex);
}

UNIMPL_NATIVE_METHOD(getActiveUniformBlockParameter)

NATIVE_METHOD(getActiveUniformBlockName, 2) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fProgram, GLuint uniformBlockIndex);
  std::string blockName;

  addBlockingToNextBatch([&] {
    GLuint program = lookupObject(fProgram);
    GLint bufSize;
    glGetActiveUniformBlockiv(program, uniformBlockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &bufSize);
    glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, NULL, &blockName[0]);
  });
  return jsi::String::createFromUtf8(runtime, blockName);
}

NATIVE_METHOD(uniformBlockBinding, 3) {
  EXJS_UNPACK_ARGV(UEXGLObjectId program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
  addToNextBatch([=] {
    glUniformBlockBinding(lookupObject(program), uniformBlockIndex, uniformBlockBinding);
  });
  return nullptr;
}

// Vertex Array Object (WebGL2)
// ----------------------------

NATIVE_METHOD(createVertexArray, 0) {
  return addFutureToNextBatch(runtime, [] {
    GLuint vertexArray;
    glGenVertexArrays(1, &vertexArray);
    return vertexArray;
  });
}

NATIVE_METHOD(deleteVertexArray, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId fVertexArray);
  addToNextBatch([=] {
    GLuint vertexArray = lookupObject(fVertexArray);
    glDeleteVertexArrays(1, &vertexArray);
  });
  return nullptr;
}

_WRAP_WEBGL2_METHOD_IS_OBJECT(VertexArray)

NATIVE_METHOD(bindVertexArray, 1) {
  EXJS_UNPACK_ARGV(UEXGLObjectId vertexArray);
  addToNextBatch([=] { glBindVertexArray(lookupObject(vertexArray)); });
  return nullptr;
}

// Extensions
// ----------

NATIVE_METHOD(getSupportedExtensions, 0) {
  return jsi::Array(runtime, 0);
}

NATIVE_METHOD(getExtension, 1) {
  return jsi::Value::null();
}

// Exponent extensions
// -------------------

NATIVE_METHOD(endFrameEXP, 0) {
  addToNextBatch([=] { setNeedsRedraw(true); });
  endNextBatch();
  flushOnGLThread();
  return nullptr;
}

NATIVE_METHOD(flushEXP, 0) {
  addBlockingToNextBatch([&] {
    // nothing, it's just a helper so that we can measure how much time some operations take
  });
  return nullptr;
}
