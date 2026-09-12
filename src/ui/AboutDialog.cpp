#include "AboutDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include <QTimer>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("About Mem-Map & Creator"));
    setFixedSize(540, 580);
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0D1117; color: #C9D1D9; font-family: 'Segoe UI', sans-serif; }"
    ));

    setupUi();
}

void AboutDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(14);

    createHeader();
    createProfileCard();
    createSocialActions();
    createTechStackChips();
    createFooter();
}

void AboutDialog::createHeader() {
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(14);

    QLabel* logoLabel = new QLabel(QStringLiteral("💾"), this);
    logoLabel->setStyleSheet(QStringLiteral("font-size: 38px;"));
    headerLayout->addWidget(logoLabel);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);

    QHBoxLayout* nameRow = new QHBoxLayout();
    QLabel* appName = new QLabel(QStringLiteral("Mem-Map"), this);
    appName->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: bold; color: #FFFFFF;"));
    QLabel* appVer = new QLabel(QStringLiteral("v1.0.0"), this);
    appVer->setStyleSheet(QStringLiteral(
        "background-color: #21262D; color: #58A6FF; font-size: 11px; font-weight: bold; "
        "padding: 2px 8px; border-radius: 4px; border: 1px solid #30363D;"
    ));
    nameRow->addWidget(appName);
    nameRow->addWidget(appVer);
    nameRow->addStretch();
    titleLayout->addLayout(nameRow);

    QLabel* appDesc = new QLabel(QStringLiteral("High-Performance Desktop Disk Space & Storage Analyzer for Windows"), this);
    appDesc->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 12px;"));
    appDesc->setWordWrap(true);
    titleLayout->addWidget(appDesc);

    headerLayout->addLayout(titleLayout);
    qobject_cast<QVBoxLayout*>(layout())->addLayout(headerLayout);
}

void AboutDialog::createProfileCard() {
    QFrame* card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #161B22; border: 1px solid #30363D; border-radius: 8px; padding: 14px; }"
    ));
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(8);

    QLabel* cardBadge = new QLabel(QStringLiteral("ARCHITECT & CREATOR"), card);
    cardBadge->setStyleSheet(QStringLiteral("color: #F0883E; font-size: 11px; font-weight: bold; letter-spacing: 1px; border: none;"));
    cardLayout->addWidget(cardBadge);

    QHBoxLayout* profileRow = new QHBoxLayout();
    profileRow->setSpacing(12);

    QLabel* avatar = new QLabel(QStringLiteral("👤"), card);
    avatar->setStyleSheet(QStringLiteral("font-size: 32px; border: none;"));
    profileRow->addWidget(avatar);

    QVBoxLayout* nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(2);

    QHBoxLayout* nameLine = new QHBoxLayout();
    QLabel* creatorName = new QLabel(m_profile.name, card);
    creatorName->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold; color: #FFFFFF; border: none;"));
    QLabel* handleLabel = new QLabel(m_profile.handle, card);
    handleLabel->setStyleSheet(QStringLiteral("color: #58A6FF; font-size: 13px; border: none;"));
    nameLine->addWidget(creatorName);
    nameLine->addWidget(handleLabel);
    nameLine->addStretch();
    nameLayout->addLayout(nameLine);

    QLabel* roleLabel = new QLabel(m_profile.role, card);
    roleLabel->setStyleSheet(QStringLiteral("color: #C9D1D9; font-size: 12px; font-weight: 500; border: none;"));
    nameLayout->addWidget(roleLabel);

    profileRow->addLayout(nameLayout);
    cardLayout->addLayout(profileRow);

    QLabel* bioLabel = new QLabel(m_profile.bio, card);
    bioLabel->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 12px; line-height: 1.4; border: none;"));
    bioLabel->setWordWrap(true);
    cardLayout->addWidget(bioLabel);

    qobject_cast<QVBoxLayout*>(layout())->addWidget(card);
}

void AboutDialog::createSocialActions() {
    QVBoxLayout* socialSection = new QVBoxLayout();
    socialSection->setSpacing(8);

    QLabel* sectionTitle = new QLabel(QStringLiteral("CONNECT & EXPLORE"), this);
    sectionTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; letter-spacing: 0.5px;"));
    socialSection->addWidget(sectionTitle);

    // Row 1: Primary Repos & Profiles
    QHBoxLayout* row1 = new QHBoxLayout();
    row1->setSpacing(8);

    QPushButton* btnGithub = new QPushButton(QStringLiteral("🐙 GitHub Profile"), this);
    btnGithub->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #FFFFFF; font-weight: bold; padding: 7px 12px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; border-color: #58A6FF; color: #58A6FF; }"
    ));
    connect(btnGithub, &QPushButton::clicked, this, &AboutDialog::openGithubProfile);
    row1->addWidget(btnGithub);

    QPushButton* btnRepo = new QPushButton(QStringLiteral("⭐ Star Repository"), this);
    btnRepo->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #238636; color: #FFFFFF; font-weight: bold; padding: 7px 12px; border-radius: 6px; border: 1px solid #2EA043; font-size: 12px; }"
        "QPushButton:hover { background-color: #2EA043; }"
    ));
    connect(btnRepo, &QPushButton::clicked, this, &AboutDialog::openGithubRepo);
    row1->addWidget(btnRepo);

    QPushButton* btnEmail = new QPushButton(QStringLiteral("✉️ Email Creator"), this);
    btnEmail->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #FFFFFF; font-weight: bold; padding: 7px 12px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; border-color: #58A6FF; color: #58A6FF; }"
    ));
    connect(btnEmail, &QPushButton::clicked, this, &AboutDialog::sendEmail);
    row1->addWidget(btnEmail);

    socialSection->addLayout(row1);

    // Row 2: Secondary Socials & Copy Action
    QHBoxLayout* row2 = new QHBoxLayout();
    row2->setSpacing(8);

    QPushButton* btnLinkedIn = new QPushButton(QStringLiteral("💼 LinkedIn"), this);
    btnLinkedIn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #161B22; color: #C9D1D9; padding: 6px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #21262D; border-color: #58A6FF; color: #58A6FF; }"
    ));
    connect(btnLinkedIn, &QPushButton::clicked, this, &AboutDialog::openLinkedIn);
    row2->addWidget(btnLinkedIn);

    QPushButton* btnTwitter = new QPushButton(QStringLiteral("🐦 X / Twitter"), this);
    btnTwitter->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #161B22; color: #C9D1D9; padding: 6px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #21262D; border-color: #58A6FF; color: #58A6FF; }"
    ));
    connect(btnTwitter, &QPushButton::clicked, this, &AboutDialog::openTwitter);
    row2->addWidget(btnTwitter);

    QPushButton* btnCopyRepo = new QPushButton(QStringLiteral("📋 Copy Repo URL"), this);
    btnCopyRepo->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #161B22; color: #C9D1D9; padding: 6px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #21262D; border-color: #58A6FF; color: #58A6FF; }"
    ));
    connect(btnCopyRepo, &QPushButton::clicked, this, &AboutDialog::copyRepoUrl);
    row2->addWidget(btnCopyRepo);

    socialSection->addLayout(row2);

    m_copyFeedbackLabel = new QLabel(this);
    m_copyFeedbackLabel->setStyleSheet(QStringLiteral("color: #3FB950; font-size: 11px; font-weight: bold;"));
    m_copyFeedbackLabel->setAlignment(Qt::AlignCenter);
    socialSection->addWidget(m_copyFeedbackLabel);

    qobject_cast<QVBoxLayout*>(layout())->addLayout(socialSection);
}

void AboutDialog::createTechStackChips() {
    QVBoxLayout* techBox = new QVBoxLayout();
    techBox->setSpacing(6);

    QLabel* techTitle = new QLabel(QStringLiteral("BUILT WITH"), this);
    techTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; letter-spacing: 0.5px;"));
    techBox->addWidget(techTitle);

    QHBoxLayout* chipsRow = new QHBoxLayout();
    chipsRow->setSpacing(6);

    const QStringList chips = {
        QStringLiteral("C++20"),
        QStringLiteral("Qt 6.6+"),
        QStringLiteral("CMake 3.28"),
        QStringLiteral("Ninja"),
        QStringLiteral("MSYS2 UCRT64"),
        QStringLiteral("MIT License")
    };

    for (const auto& chip : chips) {
        QLabel* chipLabel = new QLabel(chip, this);
        chipLabel->setStyleSheet(QStringLiteral(
            "background-color: #161B22; color: #8B949E; border: 1px solid #30363D; "
            "border-radius: 12px; padding: 3px 9px; font-size: 11px;"
        ));
        chipsRow->addWidget(chipLabel);
    }
    chipsRow->addStretch();
    techBox->addLayout(chipsRow);

    qobject_cast<QVBoxLayout*>(layout())->addLayout(techBox);
}

void AboutDialog::createFooter() {
    QHBoxLayout* footerRow = new QHBoxLayout();
    footerRow->setContentsMargins(0, 6, 0, 0);

    QLabel* copyright = new QLabel(QStringLiteral("© 2026 Suryansh Swarn. Open Source."), this);
    copyright->setStyleSheet(QStringLiteral("color: #484F58; font-size: 11px;"));
    footerRow->addWidget(copyright);

    footerRow->addStretch();

    QPushButton* btnClose = new QPushButton(QStringLiteral("Close"), this);
    btnClose->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #C9D1D9; padding: 6px 18px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; color: #FFFFFF; }"
    ));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    footerRow->addWidget(btnClose);

    qobject_cast<QVBoxLayout*>(layout())->addLayout(footerRow);
}

void AboutDialog::openGithubProfile() {
    QDesktopServices::openUrl(QUrl(m_profile.githubProfileUrl));
}

void AboutDialog::openGithubRepo() {
    QDesktopServices::openUrl(QUrl(m_profile.githubRepoUrl));
}

void AboutDialog::sendEmail() {
    QDesktopServices::openUrl(m_profile.getEmailUrl());
}

void AboutDialog::openLinkedIn() {
    QDesktopServices::openUrl(QUrl(m_profile.linkedinUrl));
}

void AboutDialog::openTwitter() {
    QDesktopServices::openUrl(QUrl(m_profile.twitterUrl));
}

void AboutDialog::copyRepoUrl() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(m_profile.githubRepoUrl);
        if (m_copyFeedbackLabel) {
            m_copyFeedbackLabel->setText(QStringLiteral("✓ Repository URL copied to clipboard: %1").arg(m_profile.githubRepoUrl));
            QTimer::singleShot(3000, this, [this]() {
                if (m_copyFeedbackLabel) {
                    m_copyFeedbackLabel->clear();
                }
            });
        }
    }
}
