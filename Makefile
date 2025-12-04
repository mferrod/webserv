NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRCS = 	sources/main.cpp \
       	sources/HttpRequest.cpp \
       	sources/httpUtils.cpp \
    	sources/ServerManager.cpp \
        sources/ServerSocket.cpp \
        sources/ClientConnection.cpp \
		sources/ConfigParser.cpp \
		sources/ConfigValidator.cpp \
		sources/ServerConfig.cpp \
		sources/Location.cpp \
		sources/HttpResponse.cpp \

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
