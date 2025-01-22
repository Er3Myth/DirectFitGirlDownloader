#pragma once
#include <iostream>


#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H



class ProgressBar {
private:
    int totalSteps;
    int currentStep;
    int barWidth;
    int spinnerFrame;
    bool showSpinner;
    bool useColors;
    std::string customPrefix;
    std::string customSuffix;

    const std::string spinnerFrames[7] = {
        "⢄","⢂","⢁","⡁","⡈","⡐","⡠"
    };

    std::string getSpinner() const;

public:
    ProgressBar(int total, int width = 50, bool enableSpinner = true, bool enableColors = true);

    // Update progress to a specific value
    void update(int step);

    // Increment progress by 1
    void incrememnt();

    // Set custom messages
    void setPrefix(const std::string& prefix);
    void setSuffix(const std::string& suffix);

    // Reset the progress bar
    void reset();

    // Draw the progress bar
    void draw();

    // Complete the progress bar
    void complete();

    // Destructor ensures we move to the next line
    ~ProgressBar();
};



#endif //PROGRESSBAR_H
