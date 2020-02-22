
TEST   =$(wildcard test*.cpp)
SOURCE :=$(wildcard *.cpp)
SOURCE := $(filter-out $(TEST),$(SOURCE))
OBJS   :=$(patsubst %.cpp,%.o,$(SOURCE))

TARGET  :=HttpServer
LIBS    := -lpthread
#INCLUDE:= -I./usr/local/lib
CFLAGS  := -std=c++11 -g -Wall -O2


TEST_1:=test_log
TEST_2:=test_mutex
TEST_3:=test_thread
TEST_4:=test_eventloop_0
TEST_5:=test_eventloop_1
TEST_6:=test_log_performance
TEST_7:=test_http_client
TEST_8:=test_client

TEST_01:=test_01
TEST_02:=test_02
TEST_03:=test_03
TEST_04:=test_04
TEST_05:=test_05
TEST_06:=test_06
TEST_07:=test_07

TEST = $(TEST_1) $(TEST_2) $(TEST_3) $(TEST_4) $(TEST_5) $(TEST_6) $(TEST_7) $(TEST_8)\
$(TEST_01) $(TEST_02) $(TEST_03) $(TEST_04) $(TEST_05) $(TEST_06) $(TEST_07)

$(TARGET) : $(OBJS) test_http.o
	g++ $(CFLAGS) -o $(TARGET) $^ $(LDFLAGS) $(LIBS)
objs : $(SOURCE)
	g++ $(CFLAGS)  -c $^ $(LIBS)

$(TEST_1):$(OBJS) $(TEST_1).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_2):$(OBJS) $(TEST_2).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_3):$(OBJS) $(TEST_3).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_4):$(OBJS) $(TEST_4).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_5):$(OBJS) $(TEST_5).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_6):$(OBJS) $(TEST_6).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_7):$(OBJS) $(TEST_7).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_8):$(OBJS) $(TEST_8).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)



$(TEST_01):$(OBJS) $(TEST_01).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_02):$(OBJS) $(TEST_02).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_03):$(OBJS) $(TEST_03).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_04):$(OBJS) $(TEST_04).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_05):$(OBJS) $(TEST_05).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_06):$(OBJS) $(TEST_06).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(TEST_07):$(OBJS) $(TEST_07).o
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)



clean:
	-rm *.o $(TARGET) $(TEST)



