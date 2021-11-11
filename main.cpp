#include <SFML/Graphics.hpp>
#include "stdio.h"
#include <string>
#include "libraries/lodepng/lodepng.h"
#include "math.h"

#define uint unsigned int

using namespace std;

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

//blows up for delta == distance (distance has to be sufficiently large)
void physics(Point * points, uint width, uint height, float delta, float distance){

	Point points_new[width * height];

	//do the calculations
	for(uint i = 0; i < width; i++){
		for(uint j = 0; j < height; j++){
			uint index = toCoord(i, j, width, height);

			Point current = points[index];

			//get needed info
			float temp = current.temperature;
			float cond = current.conductivity;
			float tempup, tempdown, tempright, templeft;
			float condup, conddown, condright, condleft;

			//check for boundary cases
			if(i == 0){
				templeft = temp;
				condleft = cond;
			}else{
				Point pointleft = points[toCoord(i - 1, j, width, height)];

				templeft = pointleft.temperature;
				condleft = pointleft.conductivity;
			}

			if(i == width - 1){
				tempright = temp;
				condright = cond;
			}else{
				Point pointright = points[toCoord(i + 1, j, width, height)];

				tempright = pointright.temperature;
				condright = pointright.conductivity;
			}


			if(j == 0){
				tempup = temp;
				condup = cond;
			}else{
				Point pointup = points[toCoord(i, j - 1, width, height)];

				tempup = pointup.temperature;
				condup = pointup.conductivity;
			}


			if(j == height - 1){
				tempdown = temp;
				conddown = cond;
			}else{
				Point pointdown = points[toCoord(i, j + 1, width, height)];

				tempdown = pointdown.temperature;
				conddown = pointdown.conductivity;
			}

			//printf("%f %f %f %f\n", condup, conddown, condleft, condright);

			//calculate energy flux
			float fluxup    = (tempup - temp)*(condup + cond)/2 * delta / distance;
			float fluxdown  = (tempdown - temp)*(conddown + cond)/2 * delta / distance;
			float fluxleft  = (templeft - temp)*(condleft + cond)/2 * delta / distance;
			float fluxright = (tempright - temp)*(condright + cond)/2 * delta / distance;

			float tempchange = fluxup + fluxdown + fluxleft + fluxright;

			//printf("Delta t (%f)\n", tempchange);

			//apply to the new value
			points_new[index].temperature = temp + tempchange;
			points_new[index].conductivity = points[index].conductivity;
		}
	}

	//return new values back to the original array
	for(uint i = 0; i < width; i++){
		for(uint j = 0; j < height; j++){
			points[toCoord(i, j, width, height)] = points_new[toCoord(i, j, width, height)];
		}
	}


}

int main(){

	//declare constants
	int WIDTH = 100;
	int HEIGHT = 100;

	float DELTA = 0.001;
	float DISTANCE = 0.01;

	//create texture and sprite for rendering
	sf::Uint8* pixels = new sf::Uint8[WIDTH*HEIGHT*4];
	unsigned char* colors = new unsigned char[WIDTH * HEIGHT * 3];

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
		p[i].temperature = 250;
		p[i].conductivity = 1;


	}

	for(int frame = 0; frame < 50000; frame++){
		physics(p, WIDTH, HEIGHT, DELTA, DISTANCE);

		printf("Frame %d at time %f s\n", frame, frame * DELTA);

		for(register int i = 0; i < WIDTH*HEIGHT*4; i += 4) {

			float temp = p[i / 4].temperature;

			if(i < WIDTH * 10 * 4){
				p[i / 4].temperature = 1000;
			}

			pixels[i] = temp * 255 / 1000;
			pixels[i+1] = 0;
			pixels[i+2] = 255 - temp * 255 / 1000;

			pixels[i+3] = 255;

			colors[i*3/4] = (int) pixels[i];
			colors[i*3/4 + 1] = (int) pixels[i + 1];
			colors[i*3/4 + 2] = (int) pixels[i + 2];
		}

		lodepng::encode(std::string("frames/frame_") + std::to_string(frame) + std::string(".png"), colors, WIDTH, HEIGHT, LCT_RGB);

		window.clear(sf::Color::Black);

		texture.update(pixels);
		window.draw(sprite);

		window.display();
	}

	int framerate = 1 / DELTA;

	printf("Rendering ...\n");
	system(std::string(("ffmpeg -y -framerate ") + std::to_string(framerate) + std::string(" -i frames/frame_%d.png frames/output.mp4 > /dev/null")).c_str());
	printf("Deleting pngs ...\n");
	system("rm -r frames/*.png");
	printf("Done.\n");

	return 0;
}
