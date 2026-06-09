#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <string>
#include <functional>
#include "../core/entity.hpp"

class QTcpSocket;

struct AgentConfig {
    ControlMode mode = ControlMode::Manual;
    std::string scriptPath;
    std::string tcpHost = "127.0.0.1";
    int tcpPort = 8080;
};

class AgentRunner : public QObject {
    Q_OBJECT
public:
    explicit AgentRunner(QObject *parent = nullptr);
    ~AgentRunner();

    void configurePlayer1(const AgentConfig& config);
    void configurePlayer2(const AgentConfig& config);

    std::string query(const std::string& stateJson, int playerNum);

    // Callbacks to log messages or errors back to the UI/Console
    using LogCallback = std::function<void(const std::string& message)>;
    void setLogCallback(LogCallback cb) {
        logCallback = cb;
    }

private:
    AgentConfig p1Config;
    AgentConfig p2Config;

    QTcpSocket* p1Socket = nullptr;
    QTcpSocket* p2Socket = nullptr;

    LogCallback logCallback;

    std::string queryScript(const std::string& stateJson, const std::string& scriptPath);
    std::string queryTCP(const std::string& stateJson, const std::string& host, int port, int playerNum);
};
