#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Custom/Mails/mailhistoryunit.h"
#include <QMainWindow>
#include <qlineedit.h>

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
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:

    void on_LogInButton_released();

    void on_EmailLine_editingFinished();

    void on_SendButton_released();

    void on_NewLetterButton_released();

    void HistoryWidgetClicked(QVector<LetterStruct> RelatedLetters);

    void on_SendReplyButton_released();

    void SelectLetters_Slot();

private:
    Ui::MainWindow* ui;
    bool isValidEmail(const QString& email);
    bool CheckEmails(const QLineEdit* Container);
    void CleanNewLetterFields();

    void SendLetter(const LetterStruct& Letter);

    // and connect the signals
    void SpawnNewHistoryUnit(const LetterStruct& Letter);
    // and connect the signals
    void SpawnNewHistoryUnit(const QVector<LetterStruct>& Letter);
    template<typename... Args>
    void SpawnNewHistoryUnit(Args&&... args);
    void PopulateMailsHistory();

    bool WriteLettersToFile(const QVector<LetterStruct>& letters, const QString& FullFileName);
    QVector<LetterStruct> ReadLettersFromFile(const QString& FullFileName);
    bool AppendLettersToFile(const QVector<LetterStruct>& Letters, const QString& FullFileName);

    void SelectFilesAndRefreshLabels();
    void ClearSelectedFilesAndRefreshLabels();

    QString m_current_user{"kormak1752@gmail.com"};
    const QString m_temp_file_path{R"*(D:\SoftServe\Temp\)*"};
    QVector<QString> m_currently_selected_files{};
    MailHistoryUnit* m_current_history_unit{};
};
#endif // MAINWINDOW_H
