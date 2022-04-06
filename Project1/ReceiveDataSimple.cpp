#include <iostream>
#include <lsl_cpp.h>
#include <vector>
#include <chrono>
#include <string>
#include <thread>

/**
 * This is a minimal example that demonstrates how a multi-channel stream (here 128ch) of a
 * particular name (here: SimpleStream) can be resolved into an inlet, and how the raw sample data &
 * time stamps are pulled from the inlet. This example does not display the obtained data.
 */



class Inlet {
private:
	const int nchannels = 8;
	std::string name = "SimpleStream";
	int samplingrate = 100;
	int max_buffered = 360;
	std::string type = "EEG";
	int max_samples = 8;
public:
	Inlet() {
	}
	void startReceive() {
		// resolve the stream of interet
		std::cout << "Now resolving streams..." << std::endl;
		std::vector<lsl::stream_info> results = lsl::resolve_stream("type", type);
		if (results.empty()) throw std::runtime_error("No stream found");

		std::cout << "Here is what was resolved: " << std::endl;
		std::cout << results[0].as_xml() << std::endl;

		// make an inlet to get data from it
		std::cout << "Now creating the inlet..." << std::endl;
		lsl::stream_inlet inlet(results[0]);

		// start receiving & displaying the data
		std::cout << "Now pulling samples..." << std::endl;

		std::vector<float> sample;
		std::vector<std::vector<float>> chunk_nested_vector;
		for (int i = 0; ; ++i) {
			// pull a single sample
			inlet.pull_sample(sample);
			printChunk(sample, inlet.get_channel_count());

			// Sleep so the outlet will have time to push some samples
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			// pull a chunk into a nested vector - easier, but slower
			/*inlet.pull_chunk(chunk_nested_vector);
			printChunk(chunk_nested_vector);*/

			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			// pull a multiplexed chunk into a flat vector
			// inlet.pull_chunk_multiplexed(sample);
			std::cout << sample.size()<<std::endl<<i;
			// printChunk(sample, inlet.get_channel_count());
			
		}

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
	const int nchannels = 8;
	std::string name = "SimpleStream";
	std::string field = "SimpleStream";
	int samplingrate = 10;
	int max_buffered = 360;
	std::string type = "EEG";
	const char* channels[8] = { "C3", "C4", "Cz", "FPz", "POz", "CPz", "O1", "O2" };
public:
	Outlet() {
	}
	void startSend() {
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
				.append_child_value("unit", "microvolts")
				.append_child_value("type", type);

			// make a new outlet
			lsl::stream_outlet outlet(info, 0, max_buffered);
			std::vector<float> sample(nchannels, 0.0);

			// Your device might have its own timer. Or you can decide how often to poll
			//  your device, as we do here.
			int32_t sample_dur_us = 1000000 / (samplingrate > 0 ? samplingrate : 100);
			auto t_start = std::chrono::high_resolution_clock::now();
			auto next_sample_time = t_start;

			// send data forever
			std::cout << "Now sending data... " << std::endl;
			double starttime = ((double)clock()) / CLOCKS_PER_SEC;
			for (unsigned t = 0;; t++) {
				// Create random data for the first 8 channels.
				for (int c = 0; c < 8; c++) sample[c] = (float)((rand() % 1500) / 500.0 - 1.5);
				// For the remaining channels, fill them with a sample counter (wraps at 1M).
				std::fill(sample.begin() + 8, sample.end(), t % 1000000);

				// Wait until the next expected sample time.
				next_sample_time += std::chrono::microseconds(sample_dur_us);
				std::this_thread::sleep_until(next_sample_time);

				// send the sample
				std::cout << sample[0] << "\t" << sample[7] << std::endl;
				// outlet.push_sample(sample);
				outlet.push_chunk_multiplexed(sample);
				
			}
			std::cout << "Stop sending data. " << std::endl;
		}
		catch (std::exception& e) { 
			std::cerr << "Got an exception: " << e.what() << std::endl;
		}
	}
};

int main(int argc, char **argv) {
	Inlet inlet;
	Outlet outlet;

	
	/*std::thread thread1(&Outlet::startSend, outlet);
	std::this_thread::sleep_for(std::chrono::milliseconds(100000));*/
	std::thread thread2(&Inlet::startReceive, inlet);
	std::this_thread::sleep_for(std::chrono::milliseconds(100000));

	return 0;
}
