
NAME	:= webserv
CFLAGS	:=  -Wall -Werror -Wextra -std=c++98 \
	#-g #-fsanitize=address,undefined \
	#-Wunreachable-code -Ofast 

SRCS	:= main.cpp ConfigParser.cpp Location.cpp ServerConfig.cpp ConfigValidator.cpp

OBJS	:= ${SRCS:.cpp=.o}
CC		:= c++

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "$(NAME) created !"

%.o: %.cpp
	@$(CC) $(CFLAGS) -o $@ -c $< 

# Regla general para compilar archivos .c a archivos .o
#$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)/$(SRC_DIR)
#	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


clean:
	@rm -rf $(OBJS)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
