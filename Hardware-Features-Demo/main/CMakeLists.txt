set(SOURCES main.c)
idf_component_register(SRCS main.c music.c fft.c mic_fft_test.c
                    INCLUDE_DIRS .
                    REQUIRES core2forAWS fatfs)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)