#pragma once
#include <vsg/all.h>
#include "VsgViewerPipe.h"
#include <vsg/io/Options.h>
#include <unordered_map>
#include <functional>
#include <sstream>

class ViewerPipeCtrl : public VsgViewerPipe
{
public:
    ViewerPipeCtrl(vsg::Trackball& rtrackball, vsg::Window& rwindow)
        : VsgViewerPipe(VsgViewerPipe::SERVER), _trackball(rtrackball), _window(rwindow)
    {
        // Initialize the command handlers
        _commandHandlers["A"] = [this](const std::string& params) { handleZoom(params, 0.1); };
        _commandHandlers["B"] = [this](const std::string& params) { handleZoom(params, -0.1); };
        _commandHandlers["R"] = [this](const std::string& params) { handleRotate(params); };
        _commandHandlers["P"] = [this](const std::string& params) { handlePan(params); };
        _commandHandlers["O"] = [this](const std::string& params) { handleKeyPress(params); };
    }

    void process()
    {
        std::string message;
        while (read(message) > 0)
        {
            std::cout << "Received message: " << message << std::endl;
            write(message);

            std::istringstream iss(message);
            std::string cmd;
            iss >> cmd;

            auto it = _commandHandlers.find(cmd);
            if (it != _commandHandlers.end())
            {
                it->second(message);
            }
            else
            {
                std::cerr << "Unknown command: " << cmd << std::endl;
            }
        }
    }

private:
    void handleZoom(const std::string& params, double zoomFactor)
    {
        _trackball.zoom(zoomFactor);
    }

    void handleRotate(const std::string& params)
    {
        _trackball.rotate(5.0 * (3.12 / 360), vsg::dvec3(0.0, 1.0, 1.0));
    }

    void handlePan(const std::string& params)
    {
        _trackball.pan(vsg::dvec2(0.1, 0.0));
    }

    void handleKeyPress(const std::string& params)
    {
        vsg::ref_ptr<vsg::KeyPressEvent> keyPressEvent = vsg::KeyPressEvent::create();
        keyPressEvent->window = &_window;
        keyPressEvent->time = vsg::clock::now();
        keyPressEvent->keyBase = vsg::KEY_o;
        keyPressEvent->keyModified = vsg::KEY_o;
        _trackball.apply(*keyPressEvent);
    }

    vsg::Trackball& _trackball;
    vsg::Window& _window;
    std::unordered_map<std::string, std::function<void(const std::string&)>> _commandHandlers;
};