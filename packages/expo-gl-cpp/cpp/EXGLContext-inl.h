#pragma once

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif
#ifdef __APPLE__
#include <OpenGLES/EAGL.h>
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif

#include <jsi/jsi.h>

template <typename Func>
jsi::Value EXGLContext::getActiveInfo(
    jsi::Runtime& runtime,
    UEXGLObjectId fProgram,
    GLuint index,
    GLenum lengthParam,
    Func glFunc) {
  if (fProgram == 0) {
    return nullptr;
  }

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
    return nullptr;
  }

  jsi::Object jsResult(runtime);
  jsResult.setProperty(runtime, "name", jsi::String::createFromAscii(runtime, name.c_str()));
  jsResult.setProperty(runtime, "size", size);
  jsResult.setProperty(runtime, "type", static_cast<double>(type));
  return jsResult;
}
