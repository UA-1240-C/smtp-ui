#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <QRegularExpression>
#include <QScrollBar>
#include "Custom/Mails/mailhistoryunit.h"
#include "SmtpClient.h"
#include "MailMessageBuilder.h"
#include <QRegularExpression>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->N_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
}

MainWindow::MainWindow(QWidget* parent, std::shared_ptr<ISXSC::SmtpClient> smtp_client)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_smtp_client(smtp_client)
{
    ui->setupUi(this);

    connect(ui->N_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
}

MainWindow::MainWindow(QWidget* parent, std::shared_ptr<ISXSC::SmtpClient> smtp_client, std::shared_ptr<ISXICI::ImapClient> imap_client)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_smtp_client(smtp_client)
    , m_imap_client(imap_client)
{
    ui->setupUi(this);

    PopulateMailsHistory();

    connect(ui->N_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_LogInButton_released()
{
    m_current_user = ui->LoginLine->text();
    m_current_password = ui->PasswordLine->text();
    m_current_server = ui->ServerLine->text();

    qsizetype colon_index = m_current_server.indexOf(':');
    try
    {
        if (colon_index == -1)
            throw std::runtime_error("Invalid server address. Please provide the port number as well.");

        auto server_part = m_current_server.toStdString().substr(0, colon_index);
        auto port_part = m_current_server.toStdString().substr(colon_index + 1);

        m_smtp_client.lock()->AsyncConnect(server_part, std::stoi(port_part)).get();
        m_smtp_client.lock()->AsyncAuthenticate(m_current_user.toStdString(), m_current_password.toStdString()).get();

        m_imap_client.lock()->AsyncConnect("localhost", 2526).get();
        m_imap_client.lock()->AsyncLogin(m_current_user.toStdString(), m_current_password.toStdString()).get();

        ui->MainPagesStack->setCurrentIndex((int)EMainPagesIndex::MainPage);
        ui->UserEmailLabel->setText(m_current_user);
    }
    catch (const std::exception& e)
    {
        m_smtp_client.lock()->Reset();
        QMessageBox messageBox;
        messageBox.critical(0,"Error",e.what());
        messageBox.setFixedSize(500,200);
        messageBox.show();
    }
}

void MainWindow::on_EmailLine_editingFinished()
{
    CheckEmails(ui->EmailLine);
}

void MainWindow::on_SendButton_released()
{
    ISXMM::MailMessageBuilder builder;
    builder.set_from(m_current_user.toStdString());
    static QRegularExpression s_split_regex("\\s*,\\s*|\\s+");

    if (!CheckEmails(ui->EmailLine)) return;

    QString recipients = ui->EmailLine->text();
    QStringList email_list = recipients.split(s_split_regex, Qt::SkipEmptyParts);

    QString letter_subject = ui->SubjectLine->text();
    builder.set_subject(letter_subject.toStdString());
    QString letter_body = ui->LetterBodyText->toPlainText();
    builder.set_body(letter_body.toStdString());

    for (const QString& recipient : email_list)
    {
        builder.add_to(recipient.toStdString());
        QDate current_date = QDate::currentDate();
        LetterStruct letter(m_current_user, recipient, current_date.toString(), letter_subject, letter_body, m_currently_selected_files);

        SendLetter(letter);
    }

    for (const QString& file_path : m_currently_selected_files)
    {
        builder.add_attachment(file_path.toStdString());
    }
    try
    {
        m_smtp_client.lock()->AsyncSendMail(builder.Build()).get();
        ui->N_FileNameLabel->setText("");
    }
    catch (const std::exception& e)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error",e.what());
        messageBox.setFixedSize(500,200);
        messageBox.show();
    }

    CleanNewLetterFields();
}


void MainWindow::on_NewLetterButton_released()
{
    ui->LetterTypeStack->setCurrentIndex((int)ELetterPagesIndex::NewLetterPage);

    CleanNewLetterFields();
}

void MainWindow::SelectLetters_Slot()
{
    FetchMail();
    PopulateMailsHistory();
}

bool MainWindow::isValidEmail(const QString& email)
{
    static QRegularExpression s_email_regex("^[\\w\\.-]+@[\\w\\.-]+\\.\\w+$");

    QRegularExpressionMatch match = s_email_regex.match(email);
    return match.hasMatch();
}


bool MainWindow::CheckEmails(const QLineEdit* Container)
{
    static QRegularExpression s_split_regex("\\s*,\\s*|\\s+");

    QString recipients = Container->text();
    QStringList email_list = recipients.split(s_split_regex, Qt::SkipEmptyParts);
    if (email_list.isEmpty())
    {
        QMessageBox::critical(this, "No recipients!", "You haven't written any recipients!");
        return false;
    }

    QStringList invalid_emails;

    for (const QString &email : email_list)
    {
        if (!isValidEmail(email))
        {
            invalid_emails += email;
        }
    }

    if (!invalid_emails.isEmpty())
    {
        QString msg = R"*(Invalid emails: ")*";
        msg += invalid_emails.join("; ");
        msg += R"*("!)*";

        QMessageBox::critical(this, "Invalid email detected!", msg);
        return false;
    }
    return true;
}

void MainWindow::PopulateMailsHistory()
{
    if (m_imap_client.expired())
    {
        return;
    }

    QLayout* mailsHistoryLayout = ui->MailHistoryScrollArea->layout();

    QLayoutItem* child;
    child = mailsHistoryLayout->takeAt(0);
    while (child)
    {
        if (child->widget())
        {
            delete child->widget();
        }

        delete child;
        child = mailsHistoryLayout->takeAt(0);
    }

    std::vector<string> Inbox = m_imap_client.lock()->getInbox();
    std::string s = "";
    for (const auto &piece : Inbox) s += piece;
    ParseImapString(s);
}

void MainWindow::SendLetter(const LetterStruct& Letter)
{

}

void MainWindow::SelectFilesAndRefreshLabels()
{
    m_currently_selected_files = QFileDialog::getOpenFileNames(nullptr, "Select Files", "", "All Files (*.*)");
    QStringList file_names;
    if (!m_currently_selected_files.isEmpty())
    {
        for (const auto& file_path : m_currently_selected_files)
        {
            QFileInfo file_info(file_path);
            QString file_name = file_info.fileName();
            file_names.append(file_name);
        }
        QString files = file_names.join("; ");

        // Haven't found a way to not just copy and paste for each label
        ui->N_FileNameLabel->setText(files);
    }
}

void MainWindow::ParseImapString(string inString)
{
    QVector<LetterStruct> letters;

    QRegularExpression regex(R"(FROM \"(.+?)\" TO \"(.+?)\" SUBJECT \"(.+?)\" SENT \"(.+?)\")");
    QRegularExpressionMatchIterator i = regex.globalMatch(QString::fromUtf8(inString.c_str()));

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            QString sender = match.captured(1);
            QString recipient = match.captured(2);
            QString subject = match.captured(3);
            QString timestamp = match.captured(4);

            // Для прикладу, body залишається порожнім і немає прикріплених файлів
            letters.append(LetterStruct(sender, recipient, timestamp, subject, "", {}));
        }
    }

    for (auto i : letters)
    {
        SpawnNewHistoryUnit(i);
    }
}

void MainWindow::HistoryWidgetClicked(QVector<LetterStruct> related_letters)
{
    if (!related_letters.isEmpty())
    {
        m_current_history_unit = qobject_cast<MailHistoryUnit*>(sender());
        if (m_current_history_unit)
        {
            ui->LetterHistoryText->setText(m_current_history_unit->GetFullTextRepresentation());
            ui->LetterTypeStack->setCurrentIndex((int)ELetterPagesIndex::ReplyPage);
        }
    }
}

void MainWindow::CleanNewLetterFields()
{
    ui->EmailLine->setText("");
    ui->SubjectLine->setText("");
    ui->LetterBodyText->setText("");
}

template<typename... Args>
void MainWindow::SpawnNewHistoryUnit(Args&&... args)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->MailHistoryScrollArea->layout());
    MailHistoryUnit* new_history_unit = nullptr;

    if (layout)
    {
        new_history_unit = new MailHistoryUnit(std::forward<Args>(args)...);
        layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        layout->insertWidget(0, new_history_unit);
    }

    if (new_history_unit)
    {
        connect(new_history_unit, SIGNAL(OnMouseReleased(QVector<LetterStruct>)), this, SLOT(HistoryWidgetClicked(QVector<LetterStruct>)));
    }
}

void MainWindow::on_ReloadHistoryButton_released()
{
    PopulateMailsHistory();
}

void MainWindow::FetchMail()
{
    m_imap_client.lock()->AsyncFetchMail("1:max", "ENVELOPE").get();
}
