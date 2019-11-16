INCLUDE(BuildPlugin)

BUILD_PLUGIN(sampler Sampler.cpp Sampler.h MOCFILES Sampler.h EMBEDDED_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
