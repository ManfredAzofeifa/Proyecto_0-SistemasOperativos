

CC = gcc

CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS = -lpthread -lm -lSDL2 -lSDL2_ttf

TARGET = puente

SRC = main.c auto.c puente.c config.c visual.c
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)



all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)



%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@



-include $(DEP)



run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)