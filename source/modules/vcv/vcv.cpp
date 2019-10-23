/*
 * This file is part of Hylia.
 *
 * Hylia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hylia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hylia.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vcv.h"
#define RACK_FLATTEN_NAMESPACES
#include "include/rack.hpp"

namespace rack {

#if 0
void appInit() {}
void appDestroy() {}
App* appGet() { return nullptr; }

void windowInit() {}
void windowDestroy() {}

void App::init() {}
App::~App() {}
#endif

int Quantity::getDisplayPrecision() {
	return 5;
}

std::string Quantity::getDisplayValueString() {
	return string::f("%.*g", getDisplayPrecision(), math::normalizeZero(getDisplayValue()));
}

void Quantity::setDisplayValueString(std::string s) {
	float v = 0.f;
	char suffix[2];
	int n = std::sscanf(s.c_str(), "%f%1s", &v, suffix);
	if (n >= 2) {
		// Parse SI prefixes
		switch (suffix[0]) {
			case 'n': v *= 1e-9f; break;
			case 'u': v *= 1e-6f; break;
			case 'm': v *= 1e-3f; break;
			case 'k': v *= 1e3f; break;
			case 'M': v *= 1e6f; break;
			case 'G': v *= 1e9f; break;
			default: break;
		}
	}
	if (n >= 1)
		setDisplayValue(v);
}

std::string Quantity::getString() {
	std::string s;
	std::string label = getLabel();
	if (!label.empty())
		s += label + ": ";
	s += getDisplayValueString() + getUnit();
	return s;
}

#if 0
Window::Window() {}
Window::~Window() {}
void Window::run() {}
void Window::screenshot(float zoom) {}
void Window::close() {}
void Window::cursorLock() {}
void Window::cursorUnlock() {}
int Window::getMods() { return{}; }
void Window::setFullScreen(bool fullScreen) {}
bool Window::isFullScreen() { return{}; }
bool Window::isFrameOverdue() { return{}; }
double Window::getMonitorRefreshRate() { return{}; }
std::shared_ptr<Font> Window::loadFont(const std::string& filename) { return{}; }
std::shared_ptr<Image> Window::loadImage(const std::string& filename) { return{}; }
std::shared_ptr<Svg> Window::loadSvg(const std::string& filename) { return{}; }
#endif

#if 0
namespace app {
ModuleWidget::ModuleWidget() {}
ModuleWidget::~ModuleWidget() {}
void ModuleWidget::draw(const DrawArgs& args) {}
void ModuleWidget::drawShadow(const DrawArgs& args) {}
void ModuleWidget::onButton(const event::Button& e) {}
void ModuleWidget::onHoverKey(const event::HoverKey& e) {}
void ModuleWidget::onDragStart(const event::DragStart& e) {}
void ModuleWidget::onDragEnd(const event::DragEnd& e) {}
void ModuleWidget::onDragMove(const event::DragMove& e) {}
void ModuleWidget::setModule(engine::Module* module) {}
void ModuleWidget::setPanel(std::shared_ptr<Svg> svg) {}
void ModuleWidget::addParam(ParamWidget* param) {}
void ModuleWidget::addOutput(PortWidget* output) {}
void ModuleWidget::addInput(PortWidget* input) {}
ParamWidget* ModuleWidget::getParam(int paramId) { return nullptr; }
PortWidget* ModuleWidget::getOutput(int outputId) { return nullptr; }
PortWidget* ModuleWidget::getInput(int inputId) { return nullptr; }
json_t* ModuleWidget::toJson() { return nullptr; }
void ModuleWidget::fromJson(json_t* rootJ) {}
void ModuleWidget::copyClipboard() {}
void ModuleWidget::pasteClipboardAction() {}
void ModuleWidget::loadAction(std::string filename) {}
void ModuleWidget::save(std::string filename) {}
void ModuleWidget::loadDialog() {}
void ModuleWidget::saveDialog() {}
void ModuleWidget::disconnect() {}
void ModuleWidget::resetAction() {}
void ModuleWidget::randomizeAction() {}
void ModuleWidget::disconnectAction() {}
void ModuleWidget::cloneAction() {}
void ModuleWidget::bypassAction() {}
void ModuleWidget::removeAction() {}
void ModuleWidget::createContextMenu() {}

SvgScrew::SvgScrew() {}
void SvgScrew::setSvg(std::shared_ptr<Svg> svg) {}
}
#endif

#if 0
namespace asset {
void init() {}
std::string system(std::string filename) { return{}; }
std::string user(std::string filename) { return{}; }
std::string plugin(plugin::Plugin* plugin, std::string filename) { return{}; }
std::string systemDir;
std::string userDir;
std::string logPath;
std::string pluginsPath;
std::string settingsPath;
std::string autosavePath;
std::string templatePath;
std::string bundlePath;
}
#endif

namespace dsp {
void minBlepImpulse(int z, int o, float* output) {
#if 0
	// Symmetric sinc array with `z` zero-crossings on each side
	int n = 2 * z * o;
	float* x = new float[n];
	for (int i = 0; i < n; i++) {
		float p = math::rescale((float) i, 0.f, (float)(n - 1), (float) - z, (float) z);
		x[i] = sinc(p);
	}

	// Apply window
	blackmanHarrisWindow(x, n);

	// Real cepstrum
	float* fx = new float[2 * n];
	RealFFT rfft(n);
	rfft.rfft(x, fx);
	// fx = log(abs(fx))
	fx[0] = std::log(std::fabs(fx[0]));
	for (int i = 1; i < n; i++) {
		fx[2 * i] = std::log(std::hypot(fx[2 * i], fx[2 * i + 1]));
		fx[2 * i + 1] = 0.f;
	}
	fx[1] = std::log(std::fabs(fx[1]));
	// Clamp values in case we have -inf
	for (int i = 0; i < 2 * n; i++) {
		fx[i] = std::fmax(-30.f, fx[i]);
	}
	rfft.irfft(fx, x);
	rfft.scale(x);

	// Minimum-phase reconstruction
	for (int i = 1; i < n / 2; i++) {
		x[i] *= 2.f;
	}
	for (int i = (n + 1) / 2; i < n; i++) {
		x[i] = 0.f;
	}
	rfft.rfft(x, fx);
	// fx = exp(fx)
	fx[0] = std::exp(fx[0]);
	for (int i = 1; i < n; i++) {
		float re = std::exp(fx[2 * i]);
		float im = fx[2 * i + 1];
		fx[2 * i] = re * std::cos(im);
		fx[2 * i + 1] = re * std::sin(im);
	}
	fx[1] = std::exp(fx[1]);
	rfft.irfft(fx, x);
	rfft.scale(x);

	// Integrate
	float total = 0.f;
	for (int i = 0; i < n; i++) {
		total += x[i];
		x[i] = total;
	}

	// Normalize
	float norm = 1.f / x[n - 1];
	for (int i = 0; i < n; i++) {
		x[i] *= norm;
	}

	std::memcpy(output, x, n * sizeof(float));

	// Cleanup
	delete[] x;
	delete[] fx;
#endif
}
}

namespace engine {
Module::Module() {
}
Module::~Module() {
	for (ParamQuantity* paramQuantity : paramQuantities) {
		if (paramQuantity)
			delete paramQuantity;
	}
}
void Module::config(int numParams, int numInputs, int numOutputs, int numLights) {
	// This method should only be called once.
	assert(params.empty() && inputs.empty() && outputs.empty() && lights.empty() && paramQuantities.empty());
	params.resize(numParams);
	inputs.resize(numInputs);
	outputs.resize(numOutputs);
	lights.resize(numLights);
	paramQuantities.resize(numParams);
	// Initialize paramQuantities
	for (int i = 0; i < numParams; i++) {
		configParam(i, 0.f, 1.f, 0.f);
	}
}

#if 0
json_t* Module::toJson() { return nullptr; }
void Module::fromJson(json_t* rootJ) {}
#endif

engine::Param* ParamQuantity::getParam() {
	assert(module);
	return &module->params[paramId];
}

#if 1
void ParamQuantity::setSmoothValue(float smoothValue) {}
float ParamQuantity::getSmoothValue() { return{}; }
void ParamQuantity::setValue(float value) {}
float ParamQuantity::getValue() { return{}; }
float ParamQuantity::getMinValue() { return{}; }
float ParamQuantity::getMaxValue() { return{}; }
float ParamQuantity::getDefaultValue() { return{}; }
float ParamQuantity::getDisplayValue() { return{}; }
void ParamQuantity::setDisplayValue(float displayValue) {}
std::string ParamQuantity::getDisplayValueString() { return{}; }
void ParamQuantity::setDisplayValueString(std::string s) {}
int ParamQuantity::getDisplayPrecision() { return{}; }
std::string ParamQuantity::getLabel() { return{}; }
std::string ParamQuantity::getUnit() { return{}; }
#endif
}

namespace plugin {
Plugin::~Plugin() {
	for (Model* model : models) {
		delete model;
	}
}
void Plugin::addModel(Model* model) {
	// Check that the model is not added to a plugin already
	assert(!model->plugin);
	model->plugin = this;
	models.push_back(model);
}
Model* Plugin::getModel(std::string slug) {
	// slug = normalizeSlug(slug);
	for (Model* model : models) {
		if (model->slug == slug) {
			return model;
		}
	}
	return NULL;
}
#if 0
void Plugin::fromJson(json_t* rootJ) {}
#endif
}

namespace string {
#if 0
std::string fromWstring(const std::wstring& s) { return{}; }
std::wstring toWstring(const std::string& s) { return{}; }
#endif
std::string f(const char* format, ...) {
	va_list args;
	va_start(args, format);
	// Compute size of required buffer
	int size = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (size < 0)
		return "";
	// Create buffer
	std::string s;
	s.resize(size);
	va_start(args, format);
	vsnprintf(&s[0], size + 1, format, args);
	va_end(args);
	return s;
}
#if 0
std::string lowercase(const std::string& s) { return{}; }
std::string uppercase(const std::string& s) { return{}; }
std::string trim(const std::string& s) { return{}; }
std::string ellipsize(const std::string& s, size_t len) { return{}; }
std::string ellipsizePrefix(const std::string& s, size_t len);
bool startsWith(const std::string& str, const std::string& prefix) { return false; }
bool endsWith(const std::string& str, const std::string& suffix) { return false; }
std::string directory(const std::string& path) { return{}; }
std::string filename(const std::string& path) { return{}; }
std::string filenameBase(const std::string& filename) { return{}; }
std::string filenameExtension(const std::string& filename) { return{}; }
std::string absolutePath(const std::string& path) { return{}; }
float fuzzyScore(const std::string& s, const std::string& query) { return false; }
std::string toBase64(const uint8_t* data, size_t len) { return{}; }
uint8_t* fromBase64(const std::string& str, size_t* outLen) { return{}; }
#endif
}

#if 0
namespace widget {
Widget::~Widget() {}
void Widget::setPosition(math::Vec pos) {}
void Widget::setSize(math::Vec size) {}
void Widget::show() {}
void Widget::hide() {}
void Widget::requestDelete() {}
math::Rect Widget::getChildrenBoundingBox() { return{}; }
math::Vec Widget::getRelativeOffset(math::Vec v, Widget* relative) { return{}; }
math::Rect Widget::getViewport(math::Rect r) { return{}; }
void Widget::addChild(Widget* child) {}
void Widget::addChildBottom(Widget* child) {}
void Widget::removeChild(Widget* child) {}
void Widget::clearChildren() {}
void Widget::step() {}
void Widget::draw(const Widget::DrawArgs& args) {}
}
#endif

}

NVGcolor nvgRGB(unsigned char r, unsigned char g, unsigned char b) { return {}; }
NVGcolor nvgRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { return {}; }

using namespace rack;

using simd::float_4;

// Accurate only on [0, 1]
template <typename T>
T sin2pi_pade_05_7_6(T x) {
	x -= 0.5f;
	return (T(-6.28319) * x + T(35.353) * simd::pow(x, 3) - T(44.9043) * simd::pow(x, 5) + T(16.0951) * simd::pow(x, 7))
	       / (1 + T(0.953136) * simd::pow(x, 2) + T(0.430238) * simd::pow(x, 4) + T(0.0981408) * simd::pow(x, 6));
}

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T expCurve(T x) {
	return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}

template <int OVERSAMPLE, int QUALITY, typename T>
struct VoltageControlledOscillator {
	bool analog = false;
	bool soft = false;
	bool syncEnabled = false;
	// For optimizing in serial code
	int channels = 0;

	T lastSyncValue = 0.f;
	T phase = 0.f;
	T freq;
	T pulseWidth = 0.5f;
	T syncDirection = 1.f;

	dsp::TRCFilter<T> sqrFilter;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sqrMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sawMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> triMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

	T sqrValue = 0.f;
	T sawValue = 0.f;
	T triValue = 0.f;
	T sinValue = 0.f;

	void setPitch(T pitch) {
		freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
	}

	void setPulseWidth(T pulseWidth) {
		const float pwMin = 0.01f;
		this->pulseWidth = simd::clamp(pulseWidth, pwMin, 1.f - pwMin);
	}

	void process(float deltaTime, T syncValue) {
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
		if (soft) {
			// Reverse direction
			deltaPhase *= syncDirection;
		}
		else {
			// Reset back to forward
			syncDirection = 1.f;
		}
		phase += deltaPhase;
		// Wrap phase
		phase -= simd::floor(phase);

		// Jump sqr when crossing 0, or 1 if backwards
		T wrapPhase = (syncDirection == -1.f) & 1.f;
		T wrapCrossing = (wrapPhase - (phase - deltaPhase)) / deltaPhase;
		int wrapMask = simd::movemask((0 < wrapCrossing) & (wrapCrossing <= 1.f));
		if (wrapMask) {
			for (int i = 0; i < channels; i++) {
				if (wrapMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = wrapCrossing[i] - 1.f;
					T x = mask & (2.f * syncDirection);
					sqrMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Jump sqr when crossing `pulseWidth`
		T pulseCrossing = (pulseWidth - (phase - deltaPhase)) / deltaPhase;
		int pulseMask = simd::movemask((0 < pulseCrossing) & (pulseCrossing <= 1.f));
		if (pulseMask) {
			for (int i = 0; i < channels; i++) {
				if (pulseMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = pulseCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sqrMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Jump saw when crossing 0.5
		T halfCrossing = (0.5f - (phase - deltaPhase)) / deltaPhase;
		int halfMask = simd::movemask((0 < halfCrossing) & (halfCrossing <= 1.f));
		if (halfMask) {
			for (int i = 0; i < channels; i++) {
				if (halfMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = halfCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sawMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Detect sync
		// Might be NAN or outside of [0, 1) range
		if (syncEnabled) {
			T deltaSync = syncValue - lastSyncValue;
			T syncCrossing = -lastSyncValue / deltaSync;
			lastSyncValue = syncValue;
			T sync = (0.f < syncCrossing) & (syncCrossing <= 1.f) & (syncValue >= 0.f);
			int syncMask = simd::movemask(sync);
			if (syncMask) {
				if (soft) {
					syncDirection = simd::ifelse(sync, -syncDirection, syncDirection);
				}
				else {
					T newPhase = simd::ifelse(sync, (1.f - syncCrossing) * deltaPhase, phase);
					// Insert minBLEP for sync
					for (int i = 0; i < channels; i++) {
						if (syncMask & (1 << i)) {
							T mask = simd::movemaskInverse<T>(1 << i);
							float p = syncCrossing[i] - 1.f;
							T x;
							x = mask & (sqr(newPhase) - sqr(phase));
							sqrMinBlep.insertDiscontinuity(p, x);
							x = mask & (saw(newPhase) - saw(phase));
							sawMinBlep.insertDiscontinuity(p, x);
							x = mask & (tri(newPhase) - tri(phase));
							triMinBlep.insertDiscontinuity(p, x);
							x = mask & (sin(newPhase) - sin(phase));
							sinMinBlep.insertDiscontinuity(p, x);
						}
					}
					phase = newPhase;
				}
			}
		}

		// Square
		sqrValue = sqr(phase);
		sqrValue += sqrMinBlep.process();

		if (analog) {
			sqrFilter.setCutoffFreq(20.f * deltaTime);
			sqrFilter.process(sqrValue);
			sqrValue = sqrFilter.highpass() * 0.95f;
		}

		// Saw
		sawValue = saw(phase);
		sawValue += sawMinBlep.process();

		// Tri
		triValue = tri(phase);
		triValue += triMinBlep.process();

		// Sin
		sinValue = sin(phase);
		sinValue += sinMinBlep.process();
	}

	T sin(T phase) {
		T v;
		if (analog) {
			// Quadratic approximation of sine, slightly richer harmonics
			T halfPhase = (phase < 0.5f);
			T x = phase - simd::ifelse(halfPhase, 0.25f, 0.75f);
			v = 1.f - 16.f * simd::pow(x, 2);
			v *= simd::ifelse(halfPhase, 1.f, -1.f);
		}
		else {
			v = sin2pi_pade_05_5_4(phase);
			// v = sin2pi_pade_05_7_6(phase);
			// v = simd::sin(2 * T(M_PI) * phase);
		}
		return v;
	}
	T sin() {
		return sinValue;
	}

	T tri(T phase) {
		T v;
		if (analog) {
			T x = phase + 0.25f;
			x -= simd::trunc(x);
			T halfX = (x >= 0.5f);
			x *= 2;
			x -= simd::trunc(x);
			v = expCurve(x) * simd::ifelse(halfX, 1.f, -1.f);
		}
		else {
			v = 1 - 4 * simd::fmin(simd::fabs(phase - 0.25f), simd::fabs(phase - 1.25f));
		}
		return v;
	}
	T tri() {
		return triValue;
	}

	T saw(T phase) {
		T v;
		T x = phase + 0.5f;
		x -= simd::trunc(x);
		if (analog) {
			v = -expCurve(x);
		}
		else {
			v = 2 * x - 1;
		}
		return v;
	}
	T saw() {
		return sawValue;
	}

	T sqr(T phase) {
		T v = simd::ifelse(phase < pulseWidth, 1.f, -1.f);
		return v;
	}
	T sqr() {
		return sqrValue;
	}

	T light() {
		return simd::sin(2 * T(M_PI) * phase);
	}
};

struct VCO : rack::engine::Module {
	enum ParamIds {
		MODE_PARAM,
		SYNC_PARAM,
		FREQ_PARAM,
		FINE_PARAM,
		FM_PARAM,
		PW_PARAM,
		PWM_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		PW_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIN_OUTPUT,
		TRI_OUTPUT,
		SAW_OUTPUT,
		SQR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		NUM_LIGHTS
	};

	VoltageControlledOscillator<16, 16, float_4> oscillators[4];
	dsp::ClockDivider lightDivider;

	VCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MODE_PARAM, 0.f, 1.f, 1.f, "Analog mode");
		configParam(SYNC_PARAM, 0.f, 1.f, 1.f, "Hard sync");
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine frequency");
		configParam(FM_PARAM, 0.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		configParam(PW_PARAM, 0.01f, 0.99f, 0.5f, "Pulse width", "%", 0.f, 100.f);
		configParam(PWM_PARAM, 0.f, 1.f, 0.f, "Pulse width modulation", "%", 0.f, 100.f);
		lightDivider.setDivision(16);
	}

	void process(const ProcessArgs& args) override {
		float freqParam = params[FREQ_PARAM].getValue() / 12.f;
		freqParam += dsp::quadraticBipolar(params[FINE_PARAM].getValue()) * 3.f / 12.f;
		float fmParam = dsp::quadraticBipolar(params[FM_PARAM].getValue());

		int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);

		for (int c = 0; c < channels; c += 4) {
			auto* oscillator = &oscillators[c / 4];
			oscillator->channels = std::min(channels - c, 4);
			oscillator->analog = params[MODE_PARAM].getValue() > 0.f;
			oscillator->soft = params[SYNC_PARAM].getValue() <= 0.f;

			float_4 pitch = freqParam;
			pitch += inputs[PITCH_INPUT].getVoltageSimd<float_4>(c);
			if (inputs[FM_INPUT].isConnected()) {
				pitch += fmParam * inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c);
			}
			oscillator->setPitch(pitch);
			oscillator->setPulseWidth(params[PW_PARAM].getValue() + params[PWM_PARAM].getValue() * inputs[PW_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f);

			oscillator->syncEnabled = inputs[SYNC_INPUT].isConnected();
			oscillator->process(args.sampleTime, inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c));

			// Set output
			if (outputs[SIN_OUTPUT].isConnected())
				outputs[SIN_OUTPUT].setVoltageSimd(5.f * oscillator->sin(), c);
			if (outputs[TRI_OUTPUT].isConnected())
				outputs[TRI_OUTPUT].setVoltageSimd(5.f * oscillator->tri(), c);
			if (outputs[SAW_OUTPUT].isConnected())
				outputs[SAW_OUTPUT].setVoltageSimd(5.f * oscillator->saw(), c);
			if (outputs[SQR_OUTPUT].isConnected())
				outputs[SQR_OUTPUT].setVoltageSimd(5.f * oscillator->sqr(), c);
		}

		outputs[SIN_OUTPUT].setChannels(channels);
		outputs[TRI_OUTPUT].setChannels(channels);
		outputs[SAW_OUTPUT].setChannels(channels);
		outputs[SQR_OUTPUT].setChannels(channels);

		// Light
		if (lightDivider.process()) {
			if (channels == 1) {
				float lightValue = oscillators[0].light()[0];
				lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 2].setBrightness(0.f);
			}
			else {
				lights[PHASE_LIGHT + 0].setBrightness(0.f);
				lights[PHASE_LIGHT + 1].setBrightness(0.f);
				lights[PHASE_LIGHT + 2].setBrightness(1.f);
			}
		}
	}
};

rack::plugin::Plugin* pluginInstance;

#if 0
struct VCOWidget : app::ModuleWidget {
	VCOWidget(VCO* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VCO-1.svg")));

// 		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
// 		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
// 		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
// 		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));
//
// 		addParam(createParam<CKSS>(Vec(15, 77), module, VCO::MODE_PARAM));
// 		addParam(createParam<CKSS>(Vec(119, 77), module, VCO::SYNC_PARAM));
//
// 		addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, VCO::FREQ_PARAM));
// 		addParam(createParam<RoundLargeBlackKnob>(Vec(23, 143), module, VCO::FINE_PARAM));
// 		addParam(createParam<RoundLargeBlackKnob>(Vec(91, 143), module, VCO::PW_PARAM));
// 		addParam(createParam<RoundLargeBlackKnob>(Vec(23, 208), module, VCO::FM_PARAM));
// 		addParam(createParam<RoundLargeBlackKnob>(Vec(91, 208), module, VCO::PWM_PARAM));
//
// 		addInput(createInput<PJ301MPort>(Vec(11, 276), module, VCO::PITCH_INPUT));
// 		addInput(createInput<PJ301MPort>(Vec(45, 276), module, VCO::FM_INPUT));
// 		addInput(createInput<PJ301MPort>(Vec(80, 276), module, VCO::SYNC_INPUT));
// 		addInput(createInput<PJ301MPort>(Vec(114, 276), module, VCO::PW_INPUT));
//
// 		addOutput(createOutput<PJ301MPort>(Vec(11, 320), module, VCO::SIN_OUTPUT));
// 		addOutput(createOutput<PJ301MPort>(Vec(45, 320), module, VCO::TRI_OUTPUT));
// 		addOutput(createOutput<PJ301MPort>(Vec(80, 320), module, VCO::SAW_OUTPUT));
// 		addOutput(createOutput<PJ301MPort>(Vec(114, 320), module, VCO::SQR_OUTPUT));
//
// 		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(99, 42.5f), module, VCO::PHASE_LIGHT));
    }
};
#endif

template <class TModule>
plugin::Model* createWidgetlessModel(const std::string& slug) {
	struct TModel : plugin::Model {
		engine::Module* createModule() override {
			engine::Module* m = new TModule;
			m->model = this;
			return m;
		}
	};

	plugin::Model* o = new TModel;
	o->slug = slug;
	return o;
}

void init(rack::plugin::Plugin* p) {
    pluginInstance = p;
    plugin::Model* modelVCO = createWidgetlessModel<VCO>("VCO");
	p->addModel(modelVCO);
}

void print_plugin_details()
{
    rack::plugin::Plugin rp;
    init(&rp);
    plugin::Model* modelVCO = rp.getModel("VCO");
    engine::Module* m = modelVCO->createModule();
    printf("plugin initiated %p %p\n", pluginInstance, m);
    for (auto p : m->paramQuantities) {
        printf("param %i %f %f %f %s %s\n", p->paramId, p->minValue, p->maxValue, p->defaultValue, p->label.c_str(), p->unit.c_str());
    }
}
