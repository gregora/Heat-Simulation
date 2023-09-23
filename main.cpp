#include <SFML/Graphics.hpp>
#include "stdio.h"
#include <string.h>
#include <string>
#include "math.h"
#include <thread>
#include <chrono>

#define uint unsigned int

using namespace std;

const uint MAX_THREADS = 4;

struct Point {
	float temperature;
	float conductivity;
};

//get index from x,y and width,height
uint toCoord(uint x, uint y, uint width, uint height){
	return x + y * width;
}

//used for debugging
void printState(Point * points, uint width, uint height){

	for(uint i = 0; i < width; i++){
		for(uint j = 0; j < height; j++){
			printf("%d ", (int) points[toCoord(i, j, width, height)].temperature);
		}
		printf("\n");

	}

	printf("\n");

}

void physicsPoint(Point * points, Point * points_new, uint width, uint height, uint i, uint j, float delta, float distance){
	uint index = toCoord(i, j, width, height);

	Point& current = points[index];

	//get needed info
	float temp = current.temperature;
	float cond = current.conductivity;
	float tempup, tempdown, tempright, templeft;

	//check for boundary cases
	if(i == 0){
		templeft = temp;
	}else{
		Point& pointleft = points[toCoord(i - 1, j, width, height)];
		templeft = pointleft.temperature;
	}

	if(i == width - 1){
		tempright = temp;
	}else{
		Point& pointright = points[toCoord(i + 1, j, width, height)];
		tempright = pointright.temperature;
	}


	if(j == 0){
		tempup = temp;
	}else{
		Point& pointup = points[toCoord(i, j - 1, width, height)];
		tempup = pointup.temperature;
	}


	if(j == height - 1){
		tempdown = temp;
	}else{
		Point& pointdown = points[toCoord(i, j + 1, width, height)];
		tempdown = pointdown.temperature;
	}

	//calculate energy flux
	float tempchange = (tempup + tempdown + templeft + tempright - 4*temp) * cond * delta / distance;

	//apply to the new value
	points_new[index].temperature = temp + tempchange;
	points_new[index].conductivity = points[index].conductivity;

}

void physicsSector(Point * points, Point * points_new, uint width, uint height, uint i_start, uint i_end, float delta, float distance){
	
	for(uint i = i_start; i < i_end; i++){
		for(uint j = 0; j < height; j++){
			physicsPoint(points, points_new, width, height, i, j, delta, distance);
		}
	}

}

void copyPointsSector(Point * points, Point * points_new, uint width, uint height, uint i_start, uint i_end){
	for(uint i = i_start; i < i_end; i++){
		for(uint j = 0; j < height; j++){
			points[toCoord(i, j, width, height)] = points_new[toCoord(i, j, width, height)];
		}
	}
}

//blows up for delta == distance (distance has to be sufficiently large)
void physics(Point * points, uint width, uint height, float delta, float distance, int num_threads = 1){

	Point points_new[width * height];

	//calculations
	std::thread* threads[num_threads];
	for(uint i = 0; i < num_threads; i++){
		threads[i] = new std::thread(physicsSector, points, &points_new[0], width, height, i * width / num_threads, (i + 1) * width / num_threads, delta, distance);
	}

	for(uint i = 0; i < num_threads; i++){
		threads[i]->join();
		delete threads[i];
	}

	//copying
	for(uint i = 0; i < num_threads; i++){
		threads[i] = new std::thread(copyPointsSector, points, &points_new[0], width, height, i * width / num_threads, (i + 1) * width / num_threads);
	}

	for(uint i = 0; i < num_threads; i++){
		threads[i]->join();
		delete threads[i];
	}



}

int main(int arg_num, char ** args){

	//declare constants
	int WIDTH = 600;
	int HEIGHT = 600;

	float DELTA = 0.001;
	float DISTANCE = 0.01;

	bool render = false;

	for(uint i = 0; i < arg_num; i++){
		if(strcmp(args[i], "-render") == 0){
			render = true;
		}
	}

	//create texture and sprite for rendering
	sf::Uint8* pixels = new sf::Uint8[WIDTH*HEIGHT*4];

	sf::Texture texture;
	texture.create(WIDTH, HEIGHT);
	sf::Sprite sprite(texture);

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Heat simulation");

	//point array for computations
	Point p[WIDTH * HEIGHT];

	//initial heat and conductivity distribution
	for(uint i = 0; i < WIDTH * HEIGHT; i++){
		//TWO DISCRETE HEAT DISTRIBUTIONS WITH DISCRETE CONDUCTIVITY
		//p[i].temperature = (i % WIDTH > WIDTH / 2)*1000;
		//p[i].conductivity = (i / HEIGHT > HEIGHT / 2) * 2 + 0;

		//p[i].temperature = (i % WIDTH > WIDTH / 2)*1000;
		//p[i].conductivity = (i / HEIGHT > HEIGHT / 2) * 1.5 + 0.5;

		//TWO DISCRETE HEAT DISTRIBUTIONS WITH HOMOGENIUS CONDUCTIVITY
		//p[i].temperature = (i % WIDTH)*(i / WIDTH) * 500 / (HEIGHT * WIDTH / 2);
		//p[i].conductivity = 1;

		//HOMOGENIUS HEAT DISTRIBUTION
		//p[i].temperature = 250;
		//p[i].conductivity = 1;

		p[i].temperature = 100;
		p[i].conductivity = 1;

	}

	for(int frame = 0; frame < 1000; frame++){
		//time physics
	    auto start = std::chrono::high_resolution_clock::now();
		physics(p, WIDTH, HEIGHT, DELTA, DISTANCE, MAX_THREADS);
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = finish - start;

		printf("Frame %d at simulation time %f s\n", frame, frame * DELTA);
		printf("Physics took %f s\n", elapsed.count());

		for(uint i = 0; i < WIDTH*HEIGHT*4; i += 4) {

			float temp = p[i / 4].temperature;

			int ind = i / 4;

			int x = ind / WIDTH - WIDTH / 2;
			int y = ind % WIDTH - HEIGHT / 2;

			if(x*x + y*y < 30){
				p[i / 4].temperature = 1000;
			}

			pixels[i] = temp * 255 / 1000;
			pixels[i+1] = 0;
			pixels[i+2] = 255 - temp * 255 / 1000;

			pixels[i+3] = 255;
		}

		texture.update(pixels);
		window.draw(sprite);

		window.display();

		if(render){
			window.capture().saveToFile("frames/frame_" + std::to_string(frame) + ".png");
		}
	}

	int framerate = 1 / DELTA;

	if(render){
		printf("Rendering ...\n");
		system(std::string(("ffmpeg -y -framerate ") + std::to_string(framerate) + std::string(" -i frames/frame_%d.png frames/output.mp4 > /dev/null")).c_str());
		printf("Deleting pngs ...\n");
		system("rm -r frames/*.png");
		printf("Done.\n");
	}


	return 0;
}
