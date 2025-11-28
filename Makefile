NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRCS = sources/main.cpp \
       sources/HttpRequest.cpp \
       sources/httpUtils.cpp \
#        sources/ServerManager.cpp \
#        sources/ServerSocket.cpp \
#        sources/ClientConnection.cpp \

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

#NAME	:= webserv
#CFLAGS	:=  -Wall -Werror -Wextra -std=c++98 \
#	#-g #-fsanitize=address,undefined \
#	#-Wunreachable-code -Ofast 
#
#SRCS	:= main.cpp ConfigParser.cpp Location.cpp ServerConfig.cpp ConfigValidator.cpp
#
#OBJS	:= ${SRCS:.cpp=.o}
#CC		:= c++
#
#all: $(NAME)
#
#$(NAME): $(OBJS)
#	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
#	@echo "$(NAME) created !"
#
#%.o: %.cpp
#	@$(CC) $(CFLAGS) -o $@ -c $< 
#
## Regla general para compilar archivos .c a archivos .o
##$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)/$(SRC_DIR)
##	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
#
#
#clean:
#	@rm -rf $(OBJS)
#
#fclean: clean
#	@rm -rf $(NAME)
#
#re: fclean all
#
#.PHONY: all clean fclean re
#