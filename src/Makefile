export CC = g++

# export CCFLAGS = -g -std=c++0x -Wall
export CCFLGS = -std=c++0x -I/home/zxxia/.virtualenvs/aurora/include/python3.6m -I/home/zxxia/.virtualenvs/aurora/include/python3.6m -Wno-unused-result -Wsign-compare -DNDEBUG -g -fwrapv -O2 -Wall -g -fstack-protector-strong -Wformat -Werror=format-security -g -flto -fuse-linker-plugin -ffat-lto-objects

# export LDFLAGS = -Lcore -L/usr/lib/python3.5/config-3.5m-x86_64-linux-gnu/ -lpython3.5 -lstdc++ -lpthread -lm
# export LDFLAGS = -Lcore -L/usr/lib/python3.6/config-3.6m-x86_64-linux-gnu/ -lpython3.6 -lstdc++ -lpthread -lm
export LDFLAGS = -Lcore -lstdc++ -lpython3.6m -lpthread -ldl -lutil -lm -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions


LIB_SOURCES = $(wildcard core/*.cpp)
LIB_SOURCES += $(wildcard pcc/monitor_interval/*.cpp)
LIB_SOURCES += $(wildcard pcc/rate_control/*.cpp)
LIB_SOURCES += $(wildcard pcc/utility/*.cpp)
LIB_SOURCES += $(wildcard pcc/*.cpp)
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)

all:
	$(MAKE) -C pcc/monitor_interval all
	$(MAKE) -C pcc/rate_control all
	$(MAKE) -C pcc/utility all
	$(MAKE) -C pcc all
	$(MAKE) -C core all
	$(MAKE) -C app all
	$(CC) $(LIB_OBJECTS) $(CCFLAGS) -shared -o core/libudt.so $(LDFLAGS)
	$(CC) app/pccserver.o $(CCFLAGS) -o app/pccserver $(LDFLAGS) -ludt
	$(CC) app/pccclient.o $(CCFLAGS) -o app/pccclient $(LDFLAGS) -ludt

clean:
	$(MAKE) -C pcc/monitor_interval clean
	$(MAKE) -C pcc/rate_control clean
	$(MAKE) -C pcc/utility clean
	$(MAKE) -C pcc clean
	$(MAKE) -C core clean
	$(MAKE) -C app clean
