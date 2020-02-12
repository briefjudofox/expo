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

inline bool EXGLContext::glIsObject(UEXGLObjectId id, GLboolean func(GLuint)) {
  GLboolean glResult;
  addBlockingToNextBatch([&] { glResult = func(lookupObject(id)); });
  return glResult == GL_TRUE;
}

inline jsi::Value EXGLContext::glUnimplemented(std::string name) {
  throw std::runtime_error("EXGL: " + name + "() isn't implemented yet!");
}

inline jsi::Value EXGLContext::getActiveInfo(
    UEXGLObjectId fProgram,
    GLuint index,
    GLenum lengthParam,
    getActiveInfoFunc glFunc) {
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
    getActiveInfoFunc(program, index, maxNameLength, &length, &size, &type, &name[0]);
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
