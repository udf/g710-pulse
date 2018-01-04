#include <iostream>
#include <string>
#include <unistd.h>
#include <math.h>

#include "ModularSpec/OpenALDataFetcher.h"
#include "ModularSpec/Spectrum.h"
#include "ModularSpec/util.h"

#include "/usr/include/hidapi/hidapi.h"

#define mssleep(ms) usleep(ms * 1000)
#define FFT_SIZE 4096
#define SAMPLE_RATE 44100
#define BARS 10

#pragma pack(push, 1)
struct LED_SET_REPORT
{
    char report_id = 0x07;
    unsigned short wasd = 0x2000;
    unsigned short other = 0;
    bool changed = false;

    void set_wasd(unsigned short v) {
        changed = changed || (this->wasd != v);
        this->wasd = v;
    }

    void set_other(unsigned short v) {
        changed = changed || (this->other != v);
        this->other = v;
    }
};
#pragma pack(pop)

void normalize(float data[], size_t length) {
    float sum = 0;
    for (size_t i = 0; i < length; ++i)
        sum += data[i];

    for (size_t i = 0; i < length; ++i) {
        data[i] /= sum;
    }
}

float weigh_bars(float bar_data[], float weights[], size_t length) {
    float out = 0;
    for (size_t i = 0; i < length; ++i) {
        out += bar_data[i] * weights[i];
    }

    return map(std::pow(2, out), 1, 2, 82, 8192);
}

int main() {
    if (hid_init()) {
        std::cerr << "failed to initialize hidapi" << std::endl;
        return 1;
    }

    LED_SET_REPORT report_data;
    hid_device *handle = nullptr;

    // find and use first device that allows us to send feature 0x07
    struct hid_device_info *devs = hid_enumerate(0x046d, 0xc24d);
    struct hid_device_info *cur_dev = devs;
    while (cur_dev) {
        handle = hid_open_path(cur_dev->path);
        if (handle) {
            if (hid_send_feature_report(handle, (unsigned char*)&report_data, 5) > 0)
                break;

            std::cerr << "failed sending data to " << cur_dev->path << std::endl;
            hid_close(handle);
        }

        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);

    if (!handle) {
        std::cerr << "Failed to find G710" << std::endl;
        return 1;
    }


    float audio_data[FFT_SIZE];
    OpenALDataFetcher audio_fetcher(
        SAMPLE_RATE,
        FFT_SIZE,
        [](const std::vector<std::string> &list) {
            // try to find "Monitor of Built-in Audio Analog Stereo"
            for (size_t i = 0; i < list.size(); ++i) {
                if (list[i].find("Monitor") != std::string::npos &&
                    list[i].find("Analog") != std::string::npos) {
                    return i;
                }
            }

            std::cerr << "failed to find audio device" << std::endl;
            return (size_t)0;
        }
    );

    Spectrum spec(FFT_SIZE);
    spec.UseLinearNormalisation(1, 100);
    spec.average_weight = 1;
    spec.scale = 0.5;

    float bar_data[BARS];
    float weights[BARS];
    for (int i = 0; i < BARS; ++i)
        weights[i] = BARS - i;
    normalize(weights, BARS);

    size_t empty_frames = 0;
    while (true) {
        if (audio_fetcher.UpdateData() == 0) {
            empty_frames++;
            if (empty_frames >= 5) {
                empty_frames = 0;
                audio_fetcher.ReloadDevice();
            }
        } else {
            empty_frames = 0;
        }

        audio_fetcher.GetData(audio_data);
        spec.Update(audio_data);

        spec.GetData(20, 200, SAMPLE_RATE, bar_data, BARS);
        report_data.set_other(round(weigh_bars(bar_data, weights, BARS)));

        spec.GetData(7000, 3000, SAMPLE_RATE, bar_data, BARS);
        report_data.set_wasd(round(weigh_bars(bar_data, weights, BARS)));

        if (report_data.changed) {
            hid_send_feature_report(handle, (unsigned char*)&report_data, 5);
            report_data.changed = false;
        }
        mssleep(20);
    }

    return 0;
}