#include <SFML/Graphics.hpp>
#include <CL/opencl.hpp>
#include <math.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <complex>
#include <thread>

static const int target_fps = 60;
//static const int window_w_init = 1280;
//static const int window_h_init = 720;
static const int window_w_init = 300;
static const int window_h_init = 300;
static const char window_name[] = "windoww";
static const int Kernel_Option_Num = 10;

static const double ZoomFactor = 0.5;

static int window_w = window_w_init;
static int window_h = window_h_init;
static int window_wh = window_w * window_h;
float powA = 2.0f;
int colorDiv = 8;
int GL_precision = 32;

std::chrono::steady_clock::time_point empt;
std::chrono::steady_clock::time_point tmr;
cl::CommandQueue queue;
const std::string kernelSource = R"(
    __kernel void mandelSet(__global const double* input_X,
                            __global const double* input_Y,
                            __global const int* options,
							__global const double* mouseCords,
							__global uchar* output
							)
    {
        int indx = get_global_id(0);

		int maxIter = options[0];
		int size_X = options[1];
		int colorDiv = options[2];
		int textureMode = options[3];

        int indx_X = indx % options[1];
        int indx_Y = indx / options[1];

        int i = maxIter;
		
		if(textureMode == 0){
			double x = 0.0;
			double y = 0.0;
			double nx = 0.0;

			double cx = input_X[indx_X];
			double cy = input_Y[indx_Y];
        
			while(i-- && (x*x + y*y) <= 5) {
				nx = (x * x - y * y) + cx;
				y = (2.0 * x * y) + cy;
				x = nx;
			}

		} else if (textureMode == 1){
			double x = input_X[indx_X];
			double y = input_Y[indx_Y];
			double nx = 0.0;

			double mouseX = mouseCords[0];
			double mouseY = mouseCords[1];
			
			while (i-- && ((x * x + y * y) <= 5)) {
				nx = (x * x - y * y) + mouseX;
				y = (2.0 * x * y) + mouseY;
				x = nx;
			}
		}

		i = maxIter - i;
		int iNorm = 0;
		if (i > maxIter) {
			output[4 * indx] = 0;
			output[4 * indx + 1] = 0;
			output[4 * indx + 2] = 0;
		} else {
			int scaledI = i * colorDiv;
			iNorm = (int)scaledI % 511;
			iNorm = iNorm <= 255 ? iNorm : 511 - iNorm;
			output[4 * indx + 0] = iNorm;
			output[4 * indx + 1] = 255 - iNorm;
			output[4 * indx + 2] = 255;			
		}
		output[4 * indx + 3] = 255;
    }
)";
void timer(std::string p = "") {
	std::chrono::steady_clock::time_point tmp = std::chrono::steady_clock::now();
	if ((tmr != empt) && (p != "")) {
		std::cout << p << " time = " << std::chrono::duration_cast<std::chrono::microseconds>(tmp - tmr).count() / 1000.0 << "[ms]" << std::endl; // µ
	}
	tmr = tmp;
}
struct complx {
	double x;
	double y;
};

//mandel
#define dnx (x * x - y * y)
#define dny (2.0 * x * y)

//adjustable mandel
//#define dnx (x * x - y * y)
//#define dny (powA * x * y)

//burning ship
//#define dnx (x * x - y * y)
//#define dny (fabs(2.0 * x * y))
int textureMode = 0; // 0 - mandel, 1 - julia
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
void OpenCL_Buffer_Setup() {
	// Create input and output buffers
	bufferInput_X = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(double) * window_w);
	bufferInput_Y = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(double) * window_h);
	bufferInput_Options = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * Kernel_Option_Num);
	bufferInput_MouseCords = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(double) * 2);
	bufferOutput = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(sf::Uint8) * window_wh * 4);

	// Create a program from the kernel source code
	cl::Program program(context, kernelSource);

	// Build the program
	program.build({ device });

	// Create the kernel
	kernel = cl::Kernel(program, "mandelSet");

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
	std::cout << deviceName << '\n';

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
	std::vector<int> output(window_wh);

	for (int i = 0; i < window_w; ++i) {
		double x = xmin + (xmax - xmin) * i / (width - 1.0);
		input_X[i] = x;
	}
	for (int i = 0; i < window_h; ++i) {
		double y = ymin + (ymax - ymin) * i / (height - 1.0);
		input_Y[i] = y;
	}
	int* Kernel_Options = new int[4]; // 0: iterations, 1: window_w, 2: colorDiv, 3: julia?
	Kernel_Options[0] = iterations;
	Kernel_Options[1] = window_w;
	Kernel_Options[2] = colorDiv;
	Kernel_Options[3] = textureMode;

	double* MouseCords = new double[2];
	MouseCords[0] = GmouseX;
	MouseCords[1] = GmouseY;

	queue.enqueueWriteBuffer(bufferInput_X, CL_TRUE, 0, sizeof(double) * window_w, input_X.data());
	queue.enqueueWriteBuffer(bufferInput_Y, CL_TRUE, 0, sizeof(double) * window_h, input_Y.data());
	queue.enqueueWriteBuffer(bufferInput_Options, CL_TRUE, 0, sizeof(int) * Kernel_Option_Num, Kernel_Options);
	queue.enqueueWriteBuffer(bufferInput_MouseCords, CL_TRUE, 0, sizeof(double) * 2, MouseCords);
	queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(window_wh));
	queue.enqueueReadBuffer(bufferOutput, CL_TRUE, 0, sizeof(sf::Uint8) * window_wh * 4, pixels.data());

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

	double oxmin = -2.4;
	double oxmax = 1.0;
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
				}
				break;
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left) {
					std::cout << "mouse Pressed bruh " << mousePressed << "\n";
					mousePressed = true;
				} else if (event.mouseButton.button == sf::Mouse::Right) {
					textureMode = 0;
					needsReTexute = true;
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
					textureMode = 1;
					needsReTexute = true;
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
				if (event.key.code == sf::Keyboard::Key::Q) {
					precision = precision != 1 ? precision / 2 : 1;
					GL_precision = precision;
				} else if (event.key.code == sf::Keyboard::Key::E) {
					precision *= 2;
					GL_precision = precision;
				} else if (event.key.code == sf::Keyboard::Key::O) {
					xmin = oxmin;
					xmax = oxmax;
					double yRange = (oxmax - oxmin) * window_h / window_w;
					ymin = -oyRange / 2;
					ymax = oyRange / 2;
					precision = 32;
					GL_precision = precision;
				} else if (event.key.code == sf::Keyboard::Key::A) {
					powA -= 0.25;
				} else if (event.key.code == sf::Keyboard::Key::D) {
					powA += 0.25;
				} else if (event.key.code == sf::Keyboard::Key::Z) {
					colorDiv = colorDiv != 1 ? colorDiv / 2 : 1;
				} else if (event.key.code == sf::Keyboard::Key::X) {
					colorDiv = colorDiv * 2;
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
			if (needsReTexute) {
				if (!resized) {
					fracTexture = mandelbrotOpenCL(window_w, window_h, xmin, xmax, ymin, ymax, precision);
					needsReTexute = false;
				}
			}
		}
		zoomBorder.setPosition(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
		if (resized) {
			OpenCL_Buffer_Setup();
			//OpenCL_setup();
			fracTexture = mandelbrotOpenCL(window_w, window_h, xmin, xmax, ymin, ymax, precision);
			mainSprite.setTexture(fracTexture, 1);
			resized = false;
		} else {
			mainSprite.setTexture(fracTexture);
		}

		window.clear(sf::Color::Green);

		window.draw(mainSprite);
		window.draw(zoomBorder);
		window.display();
	}
}
int main() {
	OpenCL_setup();
	fractalExplorer();

	return 0;
}
