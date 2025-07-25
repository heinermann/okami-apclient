#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

#include <apclient.hpp>

#include "isocket.h"

class ArchipelagoSocket : public ISocket
{
  public:
    static ArchipelagoSocket &instance();

    // ISocket interface
    void connect(const std::string &server, const std::string &slot, const std::string &password) override;
    void disconnect() override;
    bool isConnected() const override;

    void sendLocation(int64_t locationID) override;
    void gameFinished() override;
    void poll() override;
    void processMainThreadTasks() override;

    std::string getItemName(int64_t id, int player) const override;
    std::string getItemDesc(int player) const override;
    std::string getConnectionInfo() const override;
    std::string getUUID() const override;
    std::string getStatus() const override;

    bool scoutLocations(const std::list<int64_t> &locations, int createAsHint) override;

  private:
    ArchipelagoSocket() = default;

    // Thread-safe state
    std::atomic<bool> connected_{false};
    std::atomic<bool> hasAttemptedConnection_{false}; // NEW: Track if we've ever tried to connect
    mutable std::mutex clientMutex_;
    std::unique_ptr<APClient> client_;

    // Cross-thread communication
    mutable std::mutex taskMutex_;
    std::queue<std::function<void()>> mainThreadTasks_;

    // Status and timing
    mutable std::mutex statusMutex_;
    std::string currentStatus_;
    std::string uuid_;

    std::chrono::steady_clock::time_point lastPollTime_;
    std::chrono::steady_clock::time_point connectionStartTime_;
    std::chrono::steady_clock::time_point lastErrorTime_; // NEW: Rate limit errors

    // Helpers
    void queueMainThreadTask(std::function<void()> task);
    void setStatus(const std::string &status);
    void setupHandlers(const std::string &slot, const std::string &password);

    template <typename F> auto withClient(F &&func) const -> decltype(func(*client_))
    {
        std::lock_guard<std::mutex> lock(clientMutex_);
        if (!client_)
        {
            throw std::runtime_error("Client not connected");
        }
        return func(*client_);
    }

    template <typename F> auto withClient(F &&func) -> decltype(func(*client_))
    {
        std::lock_guard<std::mutex> lock(clientMutex_);
        if (!client_)
        {
            throw std::runtime_error("Client not connected");
        }
        return func(*client_);
    }

    std::string buildURI(const std::string &server) const;
};
