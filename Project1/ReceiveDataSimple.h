#include <iostream>
#include <lsl_cpp.h>
#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include <future>
/**
 * This is a minimal example that demonstrates how a multi-channel stream (here 128ch) of a
 * particular name (here: SimpleStream) can be resolved into an inlet, and how the raw sample data &
 * time stamps are pulled from the inlet. This example does not display the obtained data.
 */



class Inlet {
private:
	const int nchannels = 8;
	std::string name = "EEGStream";
	int samplingrate = 2;
	int max_buffered = 360;
	std::string type = "EEG";
	int max_samples = 8;
	lsl::stream_inlet* inlet;
	std::vector<float> sample;
	std::vector<std::vector<float>> chunk_nested_vector;
	std::chrono::high_resolution_clock::time_point next_sample_time;
	int32_t sample_dur_us = 1000000 / (samplingrate > 0 ? samplingrate : 100); //microsecond
	bool readyReceive = false;
public:
	Inlet() {
	}

	void init() {
		// resolve the stream of interet
		std::cout << "Now resolving streams..." << std::endl;
		std::vector<lsl::stream_info> results = lsl::resolve_stream("name", name);
		if (results.empty()) throw std::runtime_error("No stream found");

		std::cout << "Here is what was resolved: " << std::endl;
		std::cout << results[0].as_xml() << std::endl;

		// make an inlet to get data from it
		std::cout << "Now creating the inlet..." << std::endl;
		inlet = new lsl::stream_inlet(results[0]);
		std::cout << "Now pulling samples..." << std::endl;
		this->readyReceive = true;
	}
	void setReadyReceive(bool readyReceive = true) {
		this->readyReceive = readyReceive;
	}

	void startReceive() {
		if (this->readyReceive) {
			try {
				auto now = std::chrono::high_resolution_clock::now();

				if (now >= next_sample_time) {

					// start receiving & displaying the data
					// pull a single sample
					inlet->pull_sample(sample);
					printChunk(sample, inlet->get_channel_count());

					// Sleep so the outlet will have time to push some samples
					// std::this_thread::sleep_for(std::chrono::milliseconds(500));

					// pull a chunk into a nested vector - easier, but slower
					/*inlet->pull_chunk(chunk_nested_vector);
					printChunk(chunk_nested_vector);*/

					// std::this_thread::sleep_for(std::chrono::milliseconds(500));

					// pull a multiplexed chunk into a flat vector
					// inlet.pull_chunk_multiplexed(sample);
					// std::cout << sample.size();
					// printChunk(sample, inlet->get_channel_count());

					next_sample_time += std::chrono::microseconds(sample_dur_us);
				}

			}
			catch (std::exception& e) {
				std::cerr << "Got an exception: " << e.what() << std::endl;
			}
		}


	}

	std::vector<float> getSample() {
		return this->sample;
	}

	void printChunk(const std::vector<float>& chunk, std::size_t n_channels) {
		for (std::size_t i = 0; i < chunk.size(); ++i) {
			std::cout << chunk[i] << ' ';
			if (i % n_channels == n_channels - 1) std::cout << '\n';
		}
	}

	void printChunk(const std::vector<std::vector<float>>& chunk) {
		for (const auto& vec : chunk) printChunk(vec, vec.size());
	}
};
class Outlet {
private:
	const int nchannels = 2;
	std::string name = "ETtream";
	int samplingrate = 1;// send n sample per second
	int max_buffered = 360;
	std::string type = "ET";
	const char* channels[2] = { "posX", "posY"};
	lsl::stream_outlet *outlet;
	int32_t sample_dur_us = 1000000 / (samplingrate > 0 ? samplingrate : 100); //microsecond
	std::vector<float> sample;
	
public:
	Outlet() {
		try {
			// make a new stream_info (100 Hz)
			lsl::stream_info info(
				name, type, nchannels, samplingrate, lsl::cf_float32, std::string(name) += type);
			// add some description fields
			info.desc().append_child_value("manufacturer", "LSL");
			lsl::xml_element chns = info.desc().append_child("channels");
			for (int k = 0; k < nchannels; k++)
				chns.append_child("channel")
				.append_child_value("label", k < 8 ? channels[k] : "Chan-" + std::to_string(k + 1))
				.append_child_value("type", type);

			// make a new outlet
			outlet = new lsl::stream_outlet(info, 0, max_buffered);
			sample = std::vector<float>(nchannels, 0.0);


		}
		catch (std::exception& e) {
			std::cerr << "Got an exception: " << e.what() << std::endl;
		}
	}

	int32_t getSampleDuration() {
		return sample_dur_us;
	}

	void startSend(int posX, int posY) {
		try {
			sample[0] = posX;
			sample[1] = posY;
			// send the sample
			std::cout << sample[0] << "\t" << sample[1] << std::endl;
			// outlet.push_sample(sample);
			outlet->push_chunk_multiplexed(sample);
		}
		catch (std::exception& e) { 
			std::cerr << "Got an exception: " << e.what() << std::endl;
		}
	}
};

void send() {
	// Your device might have its own timer. Or you can decide how often to poll
			//  your device, as we do here.
	auto t_start = std::chrono::high_resolution_clock::now();// nanosecond
	auto next_sample_time = t_start;
	Outlet outlet;
	int32_t sample_dur_us = outlet.getSampleDuration();
	// send data forever
	std::cout << "Now sending data... " << std::endl;
	double starttime = ((double)clock()) / CLOCKS_PER_SEC;
	for (unsigned t = 0;; t++) {
		next_sample_time += std::chrono::microseconds(sample_dur_us);
		std::this_thread::sleep_until(next_sample_time);
		outlet.startSend(t, t);

	}
	std::cout << "Stop sending data. " << std::endl;
}

void receive(Inlet &inlet) {

	for (int i = 0;i <10;i++) {
		inlet.startReceive();
		
	}
}
void init(Inlet* inlet) {
	try {
		inlet = new Inlet();
		//inlet->startReceive();
	}
	catch (std::exception& e) {
		std::cerr << "Got an exception: " << e.what() << std::endl;
	}
	
}
Inlet* inlet;

int main(int argc, char **argv) {
	Inlet inlet;
	std::thread thread(&Inlet::init, inlet);
	// inlet.init();
	std::cout << "mainstream";
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	bool ck = false;
	for (;;) {
		if (thread.joinable()) {
			thread.join();
			inlet.setReadyReceive(true);
			ck = true;
		}
		if(ck)inlet.startReceive();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	
		
	


	//std::thread thread1(&Outlet::startSend, outlet,1,2);
	
	while (true)
	{

	}
	// std::this_thread::sleep_for(std::chrono::milliseconds(100000));
	/*std::thread thread2(&Inlet::startReceive, inlet);
	std::this_thread::sleep_for(std::chrono::milliseconds(100000));*/

	return 0;
}
