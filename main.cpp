#include "mainwindow.h"

#include <QApplication>

#include "SmtpClient.h"
#include "ImapClient.h"

#include <iostream>
int main(int argc, char *argv[])
{

    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tls_client);
    ssl_context.set_verify_mode(boost::asio::ssl::verify_none);

    // Make io_context run in a separate thread and event loop poll for new async operations
    std::thread worker([&io_context]() {
        std::cout<<"io_context thread started"<<std::endl;
        asio::io_context::work work(io_context);
        io_context.run();
        std::cout<<"io_context thread finished"<<std::endl;
    });

    std::shared_ptr<ISXSC::SmtpClient> smtp_client = std::make_shared<ISXSC::SmtpClient>(io_context, ssl_context);
    std::shared_ptr<ISXICI::ImapClient> imap_client = std::make_shared<ISXICI::ImapClient>(io_context, ssl_context);
    // imap_client->test();


    QApplication a(argc, argv);
    MainWindow w(nullptr, smtp_client, imap_client);
    w.show();
    a.exec();

    smtp_client.reset();
    io_context.stop();
    worker.join();
}
