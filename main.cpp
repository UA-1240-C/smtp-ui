#include "mainwindow.h"

#include <QApplication>

#include "SmtpClient.h"

int main(int argc, char *argv[])
{

    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tls_client);
    ssl_context.set_verify_mode(boost::asio::ssl::verify_none);

    // Make io_context run in a separate thread and event loop poll for new async operations
    std::thread worker([&io_context]() {
        asio::io_context::work work(io_context);
        io_context.run();
    });

    std::shared_ptr<ISXSC::SmtpClient> smtp_client = std::make_shared<ISXSC::SmtpClient>(io_context, ssl_context);

    QApplication a(argc, argv);
    MainWindow w(nullptr, smtp_client);
    w.show();
    return a.exec();
}
