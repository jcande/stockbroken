# building jemalloc:
JEMALLOC_PATH := jemalloc
JEMALLOC_FLAGS := -lpthread -ljemalloc -I${JEMALLOC_PATH}/include
JEMALLOC_OBJ := ${JEMALLOC_PATH}/lib/libjemalloc_pic.a
# cc app.c -o app -L${JEMALLOC_PATH}/lib -Wl,-rpath,${JEMALLOC_PATH}/lib -ljemalloc
#CC := gcc
HARDEN := -D_FORTIFY_SOURCE=2 -fstack-protector-all
LDFLAGS := -pthread -Wl,--gc-sections -Wl,-z,relro,-z,now -fPIE -pie $(HARDEN)
CFLAGS := -ggdb ${JEMALLOC_FLAGS} ${LDFLAGS} -O3 -ffunction-sections -fdata-sections -std=gnu99 -Wall $(HARDEN)

bins := stockbroken

stockbroken_OBJS := tests.o main.o runtime_library.o profile.o symbol.o utility.o

.PHONY: all clean

all: $(bins)
clean:
	$(RM) $(bins)
	$(RM) -r .obj

# Dependencies tracking
$(foreach bin,$(bins),$(eval $(bin): $(addprefix .obj/,$($(bin)_OBJS)) ${JEMALLOC_OBJ}))

$(bins):
	$(LINK.o) -o $@ $^

src := $(wildcard *.c)
obj := $(src:%.c=.obj/%.o)
dep := $(src:%.c=.obj/%.d)

$(obj) $(dep): | .obj
.obj:
	mkdir $@
.obj/%.o: %.c
	$(COMPILE.c) -MMD -MP -o $@ $<

-include $(dep)
