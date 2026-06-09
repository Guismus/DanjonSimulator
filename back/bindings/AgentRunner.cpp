#include "AgentRunner.hpp"
#include <QProcess>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QHostAddress>
#include <format>

AgentRunner::AgentRunner(QObject *parent) : QObject(parent) {
}

AgentRunner::~AgentRunner() {
    if (p1Socket) {
        p1Socket->disconnectFromHost();
        p1Socket->deleteLater();
    }
    if (p2Socket) {
        p2Socket->disconnectFromHost();
        p2Socket->deleteLater();
    }
}

void AgentRunner::configurePlayer1(const AgentConfig& config) {
    p1Config = config;
}

void AgentRunner::configurePlayer2(const AgentConfig& config) {
    p2Config = config;
}

std::string AgentRunner::query(const std::string& stateJson, int playerNum) {
    const AgentConfig& config = (playerNum == 1) ? p1Config : p2Config;

    if (config.mode == ControlMode::Manual) {
        return "Passer";
    }

    if (config.mode == ControlMode::Script) {
        return queryScript(stateJson, config.scriptPath);
    } else if (config.mode == ControlMode::TCP) {
        return queryTCP(stateJson, config.tcpHost, config.tcpPort, playerNum);
    }

    return "Passer";
}

std::string AgentRunner::queryScript(const std::string& stateJson, const std::string& scriptPath) {
    QString qScriptPath = QString::fromStdString(scriptPath);
    if (qScriptPath.isEmpty()) {
        qScriptPath = "scripts/ai_agent.py";
    }

    QProcess process;
    QStringList arguments;
    arguments << qScriptPath << QString::fromStdString(stateJson);

    process.start("python3", arguments);
    if (!process.waitForFinished(5000)) { // 5 seconds timeout
        process.kill();
        if (logCallback) {
            logCallback("❌ [Script] Timeout lors de l'exécution du script Python.");
        }
        return "Passer";
    }

    QByteArray output = process.readAllStandardOutput().trimmed();
    QByteArray errOutput = process.readAllStandardError().trimmed();

    if (process.exitCode() != 0) {
        if (logCallback) {
            logCallback(std::format("❌ [Script] Le script a échoué avec le code {} : {}", 
                                    process.exitCode(), errOutput.constData()));
        }
        return "Passer";
    }

    QJsonParseError parseError;
    QJsonDocument respDoc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (logCallback) {
            logCallback(std::format("❌ [Script] Erreur de parsing JSON de la réponse : {} | Stdout: {}", 
                                    parseError.errorString().toStdString(), output.constData()));
        }
        return "Passer";
    }

    QJsonObject respObj = respDoc.object();
    return respObj["action"].toString("Passer").toStdString();
}

std::string AgentRunner::queryTCP(const std::string& stateJson, const std::string& host, int port, int playerNum) {
    QString qHost = QString::fromStdString(host);
    if (qHost.isEmpty()) qHost = "127.0.0.1";

    QTcpSocket*& socket = (playerNum == 1) ? p1Socket : p2Socket;
    if (!socket) {
        socket = new QTcpSocket(this);
    }

    // If socket is connected to a different host/port, disconnect first
    if (socket->state() != QAbstractSocket::UnconnectedState &&
        (socket->peerAddress().toString() != qHost && socket->peerName() != qHost || socket->peerPort() != port)) {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
    }

    if (socket->state() == QAbstractSocket::UnconnectedState) {
        socket->connectToHost(qHost, port);
        if (!socket->waitForConnected(3000)) { // 3 seconds connect timeout
            if (logCallback) {
                logCallback(std::format("❌ [TCP] Impossible de se connecter au serveur {}:{}", host, port));
            }
            return "Passer";
        }
    }

    QByteArray data = QByteArray::fromStdString(stateJson) + "\n";
    socket->write(data);
    if (!socket->waitForBytesWritten(2000)) {
        if (logCallback) {
            logCallback("❌ [TCP] Erreur d'écriture sur la socket.");
        }
        return "Passer";
    }

    // Wait for response (up to 10 seconds total)
    QByteArray responseData;
    int remainingTimeMs = 10000;
    QElapsedTimer timer;
    timer.start();

    while (remainingTimeMs > 0) {
        if (socket->state() != QAbstractSocket::ConnectedState) {
            break;
        }
        if (socket->waitForReadyRead(remainingTimeMs)) {
            responseData += socket->readAll();
            if (responseData.contains('\n')) {
                break; // Received complete line
            }
        } else {
            break; // Timeout or error
        }
        remainingTimeMs = 10000 - timer.elapsed();
    }

    if (responseData.isEmpty()) {
        if (logCallback) {
            logCallback("❌ [TCP] Timeout de 10 secondes dépassé ou aucune réponse reçue.");
        }
        return "Passer";
    }

    // Clean and parse response
    QByteArray line = responseData.split('\n').first().trimmed();
    QJsonParseError parseError;
    QJsonDocument respDoc = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (logCallback) {
            logCallback(std::format("❌ [TCP] Erreur de parsing JSON du serveur : {} | Brut: {}", 
                                    parseError.errorString().toStdString(), line.constData()));
        }
        return "Passer";
    }

    QJsonObject respObj = respDoc.object();
    return respObj["action"].toString("Passer").toStdString();
}
