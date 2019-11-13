root_dir = ../
obj_dir = $(root_dir)obj/
deps_dir = $(root_dir)dep/
bin_dir = $(root_dir)bin/
binary = $(bin_dir)test

CC = gcc
CFLAGS = -O0 -g -Wall -D_GNU_SOURCE=1
LDFLAGS = 
LIBS =
sources = \
	main.c \
	json.c \
	json_lib.c \
	json_hndl.c \
	json_helpers.c \

OBJS = $(sources:%.c=$(obj_dir)%.o)
deps = $(sources:%.c=$(deps_dir)%.d)

all: $(binary)

$(binary): $(OBJS) $(GOBJS)
	@echo "linking to $@"
	@$(CC) $(LDFLAGS) $(OBJS) $(GOBJS) -o $(binary) $(LIBS)

$(obj_dir)%.o: %.c
	@echo "compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(deps)
endif

$(deps_dir)%.d: %.c
	@$(SHELL) -ec '$(CC) -MM $(CFLAGS) $(CPPFLAGS) $< \
	| sed "s/$*\\.o[ :]*/$(subst /,\\/,$@) &/g" | sed "s/$*\\.o[ :]*/$(subst /,\\/,$(obj_dir))&/" > $@'

.PHONY : clean
clean: 
	rm -f $(OBJS)
	rm -f $(deps)
	rm -f $(binary)
