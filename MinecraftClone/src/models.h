/*
 * models.h
 *
 *  Created on: Feb 23, 2016
 *      Author: root
 */

#ifndef MODELS_H_
#define MODELS_H_

#include "render.h"
#include <stdlib.h>
#include <stdio.h>

struct vao mod_floor;
struct vao mod_cube;

int loadTexturePNG(char* path, int id, int s);

void loadTextureData(int id, size_t width, size_t height, void* data, int s);

#endif /* MODELS_H_ */