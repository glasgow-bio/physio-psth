MOC_DIR = moc

OBJECTS_DIR = obj

LIBS += \
    -L/usr/local/lib \
    -lqwt \
    -lcomedi \
    -liir

TMAKE_CXXFLAGS += -fno-exceptions

SOURCES = \
    psthplot.cpp \
    dataplot.cpp \
    main.cpp \
    physio_psth.cpp

HEADERS = \
    physio_psth.h \
    psthplot.h \
    dataplot.h
