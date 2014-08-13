TARGET   = amp
LIBS     = gc readline
CPPFLAGS = -std=c++98 -Wall -W -Wpointer-arith
OBJDIR   = obj
SOURCES  = $(wildcard src/*.cpp)
OBJECTS  = $(SOURCES:%=$(OBJDIR)/%.o)
DEPENDS  = $(SOURCES:%=$(OBJDIR)/%.d)
LINKAGE  = $(LIBDIRS:%=-L%) $(LIBS:%=-l%)

ifeq ($(strip $(DEBUG)),)
  CPPFLAGS += -O3 -fomit-frame-pointer -fvisibility-inlines-hidden \
              -fvisibility=hidden -fno-rtti
  DEFINES  += NDEBUG
else
  CPPFLAGS += -ggdb
endif

CPPFLAGS += $(DEFINES:%=-D%) $(INCDIRS:%=-I%)

.PHONY : all clean

all : $(TARGET)
	@exit 0

clean :
	@rm -rf $(TARGET) $(OBJDIR)

$(TARGET) : $(OBJECTS)
	@echo " EXE    $@"
	@g++ -pipe -o $@ $(OBJECTS) $(LINKAGE) -Wl,--gc-sections
ifeq ($(strip $(DEBUG)),)
	@strip -sx -R .comment $@
endif

$(OBJDIR)/%.cpp.o : %.cpp
	@echo " G++    $@"
	@g++ -c -pipe $(CPPFLAGS) -o $@ $<

$(OBJDIR)/%.cpp.d : %.cpp
	@mkdir -p $(dir $@)
	@g++ -MM -MP -MT "$(OBJDIR)/$<.o" $(CPPFLAGS) $< > $@ \
	  || (rm -f $@; exit 1)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDS)
endif
