# Makefile.gource
#  Created on: 2015. 2. 1.
#     Author: innocentevil

ifeq ($(MK), )
	MK=mkdir
endif

ifeq ($(GIT_MOVIE_GEN_DIR),)
	GIT_MOVIE_GEN_DIR = Z:/movie/tachyos
	GIT_MOVIE_TARGET = $(GIT_MOVIE_GEN_DIR)/tachyos.mp4
	GIT_MOVIE_OBJ = $(GIT_MOVIE_GEN_DIR)/tachyos.ppm
	GIT_MOVIE_RENDERER = gource
	GIT_MOVIE_CONV = ffmpeg
endif


all : $(GIT_MOVIE_TARGET)

$(GIT_MOVIE_TARGET): $(GIT_MOVIE_OBJ)
	@echo "Generating Movie"
	$(GIT_MOVIE_CONV) -y -r 30 -f image2pipe -vcodec ppm -i $< -vcodec libx264 -preset ultrafast -crf 1 -threads 0 -bf 0 $@
	@echo ' '

$(GIT_MOVIE_OBJ):$(GIT_MOVIE_GEN_DIR)
	@echo "Rendering Movie"
	$(GIT_MOVIE_RENDERER) -a 0.01 -c 3.8 -e 0.5 --start-position 0.5 --stop-position 1.0 --seconds-per-day 0.5 --max-user-speed 80 -640x480 -r 30 -o $@
	@echo ' ' 
	
$(GIT_MOVIE_GEN_DIR) :
	$(MK) $@
	
	
clean :
	rm -rf $(GIT_MOVIE_OBJ) $(GIT_MOVIE_TARGET) 
	rmdir $(GIT_MOVIE_GEN_DIR) 

