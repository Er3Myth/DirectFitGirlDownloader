#include "ProgressBar.h"
#include <iostream>
#include <iomanip>
#include <chrono>


std::string ProgressBar::getSpinner() const {
    return spinnerFrames[spinnerFrame % 7];
}

ProgressBar::ProgressBar(int total, int width, bool enableSpinner,bool enableColors)
    : totalSteps(total),
      currentStep(0),
      barWidth(width),
      spinnerFrame(0),
      showSpinner(enableSpinner),
      useColors(enableColors),
      customPrefix(""),
      customSuffix(" Processing...") {}

void ProgressBar::update(int step) {
    currentStep = step;
    draw();
}

void ProgressBar::incrememnt() {
    update(currentStep + 1);
}

void ProgressBar::setPrefix(const std::string &prefix) {
    customPrefix = prefix;
}

void ProgressBar::setSuffix(const std::string &suffix) {
    customSuffix = suffix;
}

void ProgressBar::reset() {
    currentStep = 0;
    spinnerFrame = 0;
    draw();
}

void ProgressBar::draw() {
    if (currentStep > totalSteps) currentStep = totalSteps;

    float progress = static_cast<float>(currentStep) / totalSteps;
    int pos = static_cast<int>(progress * barWidth);

    // Clear the current line
    std::cout << "\r\033[k";

    if (!customPrefix.empty())
        std::cout << customPrefix << " ";

    if (showSpinner) {
        std::cout << (useColors ? BRIGHT_CYAN : "")
                  << getSpinner()
                  << (useColors ? RESET : "") << " ";
    }

    // Print the progress bar
    std::cout << (useColors ? CYAN : "") << "[" << (useColors ? RESET : "");

    for (int i = 0; i < barWidth; ++i) {
        if (i < pos)
            std::cout << (useColors ? CYAN : "") << "=" << (useColors ? RESET : "");
        else if (i == pos)
            std::cout << (useColors ? BRIGHT_CYAN : "") << ">" << (useColors ? RESET : "");
        else
            std::cout << " ";
    }

    std::cout << (useColors ? CYAN : "") << "]" << (useColors ? RESET : "");

    // Print percentage
    std::cout << (useColors ? BOLD + CYAN : "") << " "
             << std::fixed << std::setprecision(1) << (progress * 100.0) << "%"
             << (useColors ? RESET : "");

    // Print custom suffix if set
    if (!customSuffix.empty()) {
        std::cout << customSuffix;
    }

    std::cout.flush();
    spinnerFrame++;
}

void ProgressBar::complete() {
    update(totalSteps);
    std::cout << std::endl;
}

ProgressBar::~ProgressBar() {
    std::cout << std::endl;
}



