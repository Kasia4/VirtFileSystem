#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
void resize(uint8_t *inimg, uint8_t *outimg, uint32_t width, uint32_t height);
#ifdef __cplusplus
}
#endif

#include <stdio.h>

#include <SDL2/SDL.h>

#define HEADER_SIZE 54
#define SCALING_STEP 5

void swap(uint32_t *a, uint32_t *b)
{
	uint32_t t = *a;
	*a = *b;
	*b = t;
}

void loadImg(char *img, uint8_t **imgBPtr)
{
	uint32_t size;
	uint8_t *header[HEADER_SIZE];
	FILE *file;

	if (!(file = fopen(img, "rb")) || fread(header, 1, HEADER_SIZE, file) != HEADER_SIZE)
	{
		exit(-1);
	}

	size = *(uint32_t *)(header + 2);

	*imgBPtr = malloc(size);
	memcpy(*imgBPtr, header, HEADER_SIZE);

	if (!fread(*imgBPtr + HEADER_SIZE, 1, size - HEADER_SIZE, file))
	{
		exit(-2);
	}

	fclose(file);
}

void updateTexture(uint8_t *bmpPtr, SDL_Renderer *renderer, SDL_Texture *texture)
{
	int offset, rowsize;

	SDL_RenderClear(renderer);

	SDL_Rect imgrect = {0, 0,
						*(uint32_t *)(bmpPtr + 18),
						*(uint32_t *)(bmpPtr + 22)};
	offset = *(uint32_t *)(bmpPtr + 10);

	rowsize = imgrect.w * 3;
	if (rowsize % 4)
		rowsize += 4 - rowsize % 4;

	SDL_UpdateTexture(texture, &imgrect, bmpPtr + 54, rowsize);
}

int main(int argc, char **argv)
{
	uint8_t *image = NULL;
	uint8_t *scaledImage = (uint8_t *)calloc(1 << 30, sizeof(uint8_t));

	if (argc < 2)
		return -3;

	loadImg(argv[1], &image);

	SDL_Window *w;
	SDL_Event event;

	SDL_Init(SDL_INIT_VIDEO);

	w = SDL_CreateWindow("Skalowanie obrazka", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 800, SDL_WINDOW_SHOWN);

	SDL_Renderer *renderer = SDL_CreateRenderer(w, -1, 0);

	uint32_t globWidth, globHeight;
	int width, height;
	globWidth = width = *(uint32_t *)(image + 18);
	globHeight = height = *(uint32_t *)(image + 22);

	SDL_Texture *texture = SDL_CreateTexture(renderer,
											 SDL_PIXELFORMAT_BGR24,
											 SDL_TEXTUREACCESS_TARGET,
											 globWidth, globHeight);

	updateTexture(image, renderer, texture);

	SDL_Rect texRect = {0, 0, globWidth, globHeight};

	SDL_RenderCopyEx(renderer, texture, &texRect, &texRect, 0.0, NULL, SDL_FLIP_VERTICAL);

	SDL_RenderPresent(renderer);
	do
	{
		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
				{
					SDL_DestroyWindow(w);
					break;
				}
			}
			else if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT ||
					event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN)

				{
					switch (event.key.keysym.sym)
					{
					case SDLK_LEFT:
						globWidth -= SCALING_STEP;
						break;
					case SDLK_RIGHT:
						globWidth += SCALING_STEP;
						//++texRect.w;
						break;
					case SDLK_UP:
						globHeight -= SCALING_STEP;
						//--texRect.h;
						break;
					case SDLK_DOWN:
						globHeight += SCALING_STEP;
						//++texRect.h;
						break;
					default:
						break;
					}
					resize(image, scaledImage, globWidth, globHeight);
					printf("%u %u\n", globWidth, globHeight);

					SDL_RenderClear(renderer);

					SDL_Rect texRect = {0, 0, globWidth, globHeight};
					SDL_Texture *texture = SDL_CreateTexture(renderer,
															 SDL_PIXELFORMAT_BGR24,
															 SDL_TEXTUREACCESS_TARGET,
															 globWidth, globHeight);
					updateTexture(scaledImage, renderer, texture);
					SDL_RenderCopyEx(renderer, texture, &texRect, &texRect, 0.0, NULL, SDL_FLIP_VERTICAL);
					SDL_RenderPresent(renderer);
				}
			}
		}

	} while (1);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(texture);
	SDL_Quit();

	free(image);

	return 0;
}
