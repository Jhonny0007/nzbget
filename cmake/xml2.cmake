set(ROOT ${CMAKE_BINARY_DIR}/xml2/)

ExternalProject_add(
	xml2
	PREFIX xml2
	URL https://gitlab.gnome.org/GNOME/libxml2/-/archive/v2.12.6/libxml2-v2.12.6.tar.gz
	TLS_VERIFY TRUE
	BUILD_IN_SOURCE TRUE
	GIT_SHALLOW TRUE
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	CONFIGURE_COMMAND cmake -S ${ROOT}src/xml2/ -b ${ROOT}src/xml2/build
	BUILD_COMMAND cmake --build ${ROOT}src/xml2/build
	INSTALL_COMMAND cmake --install
)

set(LIBS ${LIBS} ${ROOT}build/lib/libxml2.a)
set(INCLUDES ${INCLUDES} ${ROOT}build/include/)
