MOC_DIR = moc

OBJECTS_DIR = obj

LIBS += \
    -lqwt \
    -lcomedi

TMAKE_CXXFLAGS += -fno-exceptions

SOURCES = \
    psthplot.cpp \
    dataplot.cpp \
    main.cpp \
    physio-psth.cpp

HEADERS = \
    physio-psth.h \
    psthplot.h \
    dataplot.h
