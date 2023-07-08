#include <SFML/Graphics.hpp>
#include <CL/opencl.hpp>
#include <math.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <complex>
#include <thread>
#include <fstream>
#include <sstream>

static const int target_fps = 60;
//static const int window_w_init = 1280;
//static const int window_h_init = 720;
static const int window_w_init = 300;
static const int window_h_init = 300;
static const char window_name[] = "windoww";
static const int Kernel_Option_Num = 6;

static const double ZoomFactor = 0.7;

static int window_w = window_w_init;
static int window_h = window_h_init;
static int window_wh = window_w * window_h;
float powA = 2.0f;
int colorDiv = 8;
int GL_precision = 32;

int textureMode = 1; // 1 - mandel , 2 - ship
int juliaMode = 0; // 0 - default , 1 - julia
double GmouseX = 0.0;
double GmouseY = 0.0;

cl::Kernel kernel;
cl::Buffer bufferInput_X;
cl::Buffer bufferInput_Y;
cl::Buffer bufferInput_Options;
cl::Buffer bufferInput_MouseCords;
cl::Buffer bufferOutput;
cl::Device device;
cl::Context context;

std::chrono::steady_clock::time_point empt;
std::chrono::steady_clock::time_point tmr;
cl::CommandQueue queue;
std::string kernelSourceFromFile = "";
void timer(std::string p = "") {
	std::chrono::steady_clock::time_point tmp = std::chrono::steady_clock::now();
	if ((tmr != empt) && (p != "")) {
		std::cout << p << " time = " << std::chrono::duration_cast<std::chrono::microseconds>(tmp - tmr).count() / 1000.0 << "[ms]" << std::endl; // µ
	}
	tmr = tmp;
}
//adjustable mandel
//#define dnx (x * x - y * y)
//#define dny (powA * x * y)
std::string readFileToString(const std::string& filePath) {
	std::ifstream file(filePath);
	std::stringstream buffer;

	if (file) {
		buffer << file.rdbuf();
		file.close();
	} else {
		std::cout << "Failed to open the file: " << filePath << std::endl;
	}

	return buffer.str();
}
void OpenCL_Buffer_Setup() {
	// Create input and output buffers
	bufferInput_X = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(double) * window_w);
	bufferInput_Y = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(double) * window_h);
	bufferInput_Options = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * Kernel_Option_Num);
	bufferInput_MouseCords = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(double) * 2);
	bufferOutput = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(sf::Uint8) * window_wh * 4);

	// Create a program from the kernel source code
	//cl::Program program(context, kernelSource);
	//if (kernelSourceFromFile == "")
	kernelSourceFromFile = readFileToString("kernel.cl");

	cl::Program program(context, kernelSourceFromFile);
	// Build the program
	program.build({ device });

	// Create the kernel
	kernel = cl::Kernel(program, "fractalSet");

	// Set kernel arguments
	kernel.setArg(0, bufferInput_X);
	kernel.setArg(1, bufferInput_Y);
	kernel.setArg(2, bufferInput_Options);
	kernel.setArg(3, bufferInput_MouseCords);
	kernel.setArg(4, bufferOutput);
}
void OpenCL_setup() {
	// Get available platforms
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	// Select the first platform
	cl::Platform platform = platforms[0];

	// Get available devices
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

	// Select the first device
	device = devices[0];

	std::string deviceName;
	device.getInfo(CL_DEVICE_NAME, &deviceName);
	std::cout << "code running on: " << deviceName << '\n';

	// Create a context
	context = cl::Context(device);

	// Create a command queue
	queue = cl::CommandQueue(context, device);

	OpenCL_Buffer_Setup();
}
sf::Texture mandelbrotOpenCL(int width, int height, double xmin, double xmax, double ymin, double ymax, int iterations) {
	timer();
	sf::Texture texture;
	texture.create(width, height);
	printf("w: %d, h: %d, prec: %d, powA: %f, colorDiv: %d\n", width, height, GL_precision, powA, colorDiv);

	//sf::Uint8* pixels = new sf::Uint8[width * height * 4];
	std::vector<sf::Uint8> pixels(window_wh * 4);

	std::vector<double> input_X(window_w);
	std::vector<double> input_Y(window_h);

	for (int i = 0; i < window_w; ++i) {
		double x = xmin + (xmax - xmin) * i / (width - 1.0);
		input_X[i] = x;
	}
	for (int i = 0; i < window_h; ++i) {
		double y = ymin + (ymax - ymin) * i / (height - 1.0);
		input_Y[i] = y;
	}
	int* Kernel_Options = new int[Kernel_Option_Num]; // 0: iterations, 1: window_w, 2: colorDiv, 3: julia?, 4: texuteMode
	Kernel_Options[0] = iterations;
	Kernel_Options[1] = window_w;
	Kernel_Options[2] = colorDiv;
	Kernel_Options[3] = juliaMode;
	Kernel_Options[4] = textureMode;
	Kernel_Options[5] = *(int*)&powA;

	double* MouseCords = new double[2];
	MouseCords[0] = GmouseX;
	MouseCords[1] = GmouseY;

	queue.enqueueWriteBuffer(bufferInput_X, CL_TRUE, 0, sizeof(double) * window_w, input_X.data());
	queue.enqueueWriteBuffer(bufferInput_Y, CL_TRUE, 0, sizeof(double) * window_h, input_Y.data());
	queue.enqueueWriteBuffer(bufferInput_Options, CL_TRUE, 0, sizeof(int) * Kernel_Option_Num, Kernel_Options);
	queue.enqueueWriteBuffer(bufferInput_MouseCords, CL_TRUE, 0, sizeof(double) * 2, MouseCords);
	queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(window_wh));
	queue.enqueueReadBuffer(bufferOutput, CL_TRUE, 0, sizeof(sf::Uint8) * window_wh * 4, pixels.data());
	queue.finish();

	texture.update(pixels.data(), width, height, 0, 0);
	timer("frame done");
	return texture;
}

void resize_window(sf::RenderWindow& window, sf::RenderTexture& rt, const sf::ContextSettings& settings, int w, int h) {
	window_w = w;
	window_h = h;
	window_wh = window_w * window_h;
	window.setView(sf::View(sf::FloatRect(0, 0, (float)w, (float)h)));
}
void make_window(sf::RenderWindow& window, sf::RenderTexture& rt, const sf::ContextSettings& settings, bool is_fullscreen) {
	window.close();
	sf::VideoMode screenSize;
	if (is_fullscreen) {
		screenSize = sf::VideoMode::getDesktopMode();
		window.create(screenSize, window_name, sf::Style::Fullscreen, settings);
	} else {
		screenSize = sf::VideoMode(window_w_init, window_h_init, 24);
		window.create(screenSize, window_name, sf::Style::Resize | sf::Style::Close, settings);
	}
	resize_window(window, rt, settings, screenSize.width, screenSize.height);
	window.setFramerateLimit(target_fps);
	//window.setVerticalSyncEnabled(true);
	window.setKeyRepeatEnabled(false);
	window.requestFocus();
}
void updateXYRange(sf::Vector2i& mousePos, float& delta, double& xmax, double& xmin, double& ymax, double& ymin) {
	double xrange = xmax - xmin;
	double yrange = ymax - ymin;

	double mouseXpos = mousePos.x / ((double)window_w) * xrange + xmin;
	double mouseYpos = mousePos.y / ((double)window_h) * yrange + ymin;

	if (delta <= 0) { // zoom out
		xmin = xmin + (mouseXpos - xmin) * (1.0 - 1 / ZoomFactor);
		xmax = xmax - (xmax - mouseXpos) * (1.0 - 1 / ZoomFactor);
		ymin = ymin + (mouseYpos - ymin) * (1.0 - 1 / ZoomFactor);
		ymax = ymax - (ymax - mouseYpos) * (1.0 - 1 / ZoomFactor);
	} else { // zoom in
		xmin = xmin + (mouseXpos - xmin) * (1.0 - ZoomFactor);
		xmax = xmax - (xmax - mouseXpos) * (1.0 - ZoomFactor);
		ymin = ymin + (mouseYpos - ymin) * (1.0 - ZoomFactor);
		ymax = ymax - (ymax - mouseYpos) * (1.0 - ZoomFactor);
	}

	printf("cords: %d %d, x:%lf-%lf, y:%lf-%lf\n", mousePos.x, mousePos.y, xmin, xmax, ymin, ymax);
}
void fractalExplorer() {
	printf("started\n");

	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 4;
	settings.majorVersion = 3;
	settings.minorVersion = 0;

	sf::RenderWindow window;
	sf::RenderTexture renderTexture;
	bool is_fullscreen = false;
	bool toggle_fullscreen = false;
	make_window(window, renderTexture, settings, is_fullscreen);

	sf::RectangleShape zoomBorder(sf::Vector2f(window_w / 8, window_h / 8));
	zoomBorder.setFillColor(sf::Color(0, 0, 0, 0));
	zoomBorder.setOutlineColor(sf::Color(255, 255, 255, 128));
	zoomBorder.setOutlineThickness(1.0f);
	zoomBorder.setOrigin(sf::Vector2f(zoomBorder.getSize().x / 2, zoomBorder.getSize().y / 2));

	sf::Sprite mainSprite;
	sf::Texture fracTexture;

	const double oxmin = -2.4;
	const double oxmax = 1.0;
	double oyRange = (oxmax - oxmin) * window_h / window_w;
	double oymin = -oyRange / 2;
	double oymax = oyRange / 2;

	double xmin = oxmin;
	double xmax = oxmax;
	double ymin = oymin;
	double ymax = oymax;
	int precision = GL_precision;

	fracTexture = mandelbrotOpenCL(window_w, window_h, xmin, xmax, ymin, ymax, precision);

	bool mousePressed = false;
	std::vector<sf::Vertex> linePoints;
	bool middleMousePressed = false;

	while (window.isOpen()) {
		sf::Event event;
		bool resized = false;

		while (window.pollEvent(event)) {
			bool needsReTexute = false;
			switch (event.type) {
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::MouseButtonReleased:
					if (event.mouseButton.button == sf::Mouse::Left) {
						std::cout << "mouse Released bruh " << mousePressed << "\n";
						mousePressed = false;
					} else if (event.mouseButton.button == sf::Mouse::Middle) {
						middleMousePressed = false;
					}
					break;
				case sf::Event::MouseButtonPressed:
					if (event.mouseButton.button == sf::Mouse::Left) {
						std::cout << "mouse Pressed bruh " << mousePressed << "\n";
						mousePressed = true;
					} else if (event.mouseButton.button == sf::Mouse::Right) {
						juliaMode = 0;
						needsReTexute = true;
					} else if (event.mouseButton.button == sf::Mouse::Middle) {
						middleMousePressed = true;
					}
				case sf::Event::MouseMoved:
					if (mousePressed) {
						double xrange = xmax - xmin;
						double yrange = ymax - ymin;
						sf::Vector2i mousePos = sf::Mouse::getPosition(window);
						double mouseXpos = mousePos.x / ((double)window_w) * xrange + xmin;
						double mouseYpos = mousePos.y / ((double)window_h) * yrange + ymin;
						GmouseX = mouseXpos;
						GmouseY = mouseYpos;

						std::cout << "moved x: " << GmouseX << ", y: " << GmouseY << "\n";
						juliaMode = 1;
						needsReTexute = true;
					} else if (middleMousePressed && textureMode == 1) {
						double xrange = xmax - xmin;
						double yrange = ymax - ymin;
						sf::Vector2i mousePos = sf::Mouse::getPosition(window);
						linePoints.clear();
						linePoints.push_back(sf::Vertex(sf::Vector2f(mousePos.x, mousePos.y)));
						double cx = mousePos.x / ((double)window_w) * xrange + xmin;
						double cy = mousePos.y / ((double)window_h) * yrange + ymin;
						double xd = cx;
						double yd = cy;
						for (int line_i = 0; line_i < precision; line_i++) {
							double nxd = (xd * xd - yd * yd) + cx;
							double nyd = (2 * xd * yd) + cy;
							xd = nxd;
							yd = nyd;
							double newMouseX = (xd - xmin) * ((double)window_w) / xrange;
							double newMouseY = (yd - ymin) * ((double)window_h) / yrange;
							linePoints.push_back(sf::Vertex(sf::Vector2f(newMouseX, newMouseY)));
							linePoints.push_back(sf::Vertex(sf::Vector2f(newMouseX, newMouseY)));
						}
					}
					break;
				case sf::Event::Resized:
					resize_window(window, renderTexture, settings, event.size.width, event.size.height);
					oyRange = (xmax - xmin) * window_h / window_w;
					ymin = -oyRange / 2;
					ymax = oyRange / 2;
					needsReTexute = true;
					resized = true;
					break;
				case sf::Event::KeyReleased:
					switch (event.key.code) {
						case sf::Keyboard::Key::Q:
							precision = precision != 1 ? precision / 2 : 1;
							GL_precision = precision;
							break;
						case sf::Keyboard::Key::E:
							precision *= 2;
							GL_precision = precision;
							break;
						case sf::Keyboard::Key::O:
							xmin = oxmin;
							xmax = oxmax;
							ymin = -oyRange / 2;
							ymax = oyRange / 2;
							precision = 32;
							colorDiv = 8;
							GL_precision = precision;
							linePoints.clear();
							break;
						case sf::Keyboard::Key::A:
							powA -= 0.25;
							break;
						case sf::Keyboard::Key::D:
							powA += 0.25;
							break;
						case sf::Keyboard::Key::Z:
							colorDiv = colorDiv != 1 ? colorDiv / 2 : 1;
							break;
						case sf::Keyboard::Key::X:
							colorDiv = colorDiv * 2;
							break;
						case sf::Keyboard::Num1:
							textureMode = 1;
							break;
						case sf::Keyboard::Num2:
							textureMode = 2;
							break;
						case sf::Keyboard::Num3:
							textureMode = 3;
							break;
						case sf::Keyboard::Num4:
							textureMode = 4;
							break;
						case sf::Keyboard::Num5:
							textureMode = 5;
							break;
						case sf::Keyboard::Num6:
							textureMode = 6;
							break;
						case sf::Keyboard::Num0:
							resized = true;
							break;
					}
					needsReTexute = true;
					break;
				case sf::Event::MouseWheelScrolled:
					sf::Vector2i mousePos = sf::Mouse::getPosition(window);
					float delta = event.mouseWheelScroll.delta;
					updateXYRange(mousePos, delta, xmax, xmin, ymax, ymin);

					needsReTexute = true;
					break;
			}
			if (needsReTexute && !resized) {
				fracTexture = mandelbrotOpenCL(window_w, window_h, xmin, xmax, ymin, ymax, precision);
				needsReTexute = false;
				linePoints.clear();
			}
		}
		zoomBorder.setPosition(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
		if (resized) {
			OpenCL_Buffer_Setup();
			//OpenCL_setup();
			fracTexture = mandelbrotOpenCL(window_w, window_h, xmin, xmax, ymin, ymax, precision);
			mainSprite.setTexture(fracTexture, 1);
			resized = false;
			linePoints.clear();
		} else {
			mainSprite.setTexture(fracTexture);
		}

		window.clear(sf::Color::Green);

		window.draw(mainSprite);
		window.draw(zoomBorder);
		window.draw(linePoints.data(), linePoints.size(), sf::Lines);
		window.display();
	}
}
int main() {
	OpenCL_setup();
	fractalExplorer();

	return 0;
}
