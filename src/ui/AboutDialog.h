#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include "core/CreatorProfile.h"

/**
 * @brief Modal dialog presenting the creator profile, social media links,
 * GitHub repository, and project specifications.
 */
class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override = default;

private slots:
    void openGithubProfile();
    void openGithubRepo();
    void sendEmail();
    void openLinkedIn();
    void openTwitter();
    void copyRepoUrl();

private:
    void setupUi();
    void createHeader();
    void createProfileCard();
    void createSocialActions();
    void createTechStackChips();
    void createFooter();

    CreatorProfile m_profile;
    QLabel* m_copyFeedbackLabel{nullptr};
};
