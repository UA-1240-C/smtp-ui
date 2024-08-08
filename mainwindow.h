#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Custom/Mails/mailhistoryunit.h"
#include <QMainWindow>
#include <qlineedit.h>

#include <memory>
#include <boost/asio.hpp>

#include <SmtpClient.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum class EMainPagesIndex : uint8_t
{
    LoginPage = 0,
    MainPage = 1
};

enum class ELetterPagesIndex : uint8_t
{
    NewLetterPage = 0,
    ReplyPage = 1
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent=nullptr);
    MainWindow(QWidget *parent, std::shared_ptr<ISXSC::SmtpClient> smtp_client);
    ~MainWindow();
    std::weak_ptr<ISXSC::SmtpClient> m_smtp_client;

private slots:

    void on_LogInButton_released();

    void on_AttachFileButton_released();

    void on_EmailLine_editingFinished();

    void on_SendButton_released();

    void on_NewLetterButton_released();

    void HistoryWidgets(QVector<LetterStruct> RelatedLetters);

    void on_SendReplyButton_released();

private:
    Ui::MainWindow *ui;
    bool isValidEmail(const QString &email);
    bool CheckEmails(const QLineEdit* Container);
    void CleanNewLetterFields();

    // and connect the signals
    void SpawnNewHistoryUnit(const LetterStruct& Letter);
    // and connect the signals
    void SpawnNewHistoryUnit(const QVector<LetterStruct>& Letter);
    void PopulateMailsHistory();

    bool WriteLettersToFile(const QVector<LetterStruct>& Letters, const QString& FullFileName);
    QVector<LetterStruct> ReadLettersFromFile(const QString& FullFileName);

    QString m_current_user{"kormak1752@gmail.com"};
    const QString m_temp_file_path{R"*(D:\SoftServe\Temp\)*"};
    QString m_temp_current_history_unit{};
};
#endif // MAINWINDOW_H
