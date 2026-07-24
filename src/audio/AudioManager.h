#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QString>

class AudioManager : public QObject {
    Q_OBJECT
public:
    static AudioManager& instance();

    void initialize(const QString& soundsDir);

    void playSfx(const QString& name);
    void playBgm(const QString& name);
    void stopBgm();

    void setSfxVolume(float v);
    void setBgmVolume(float v);
    void setSfxMuted(bool muted);
    void setBgmMuted(bool muted);

    float sfxVolume() const { return m_sfxVolume; }
    float bgmVolume() const { return m_bgmVolume; }
    bool sfxMuted() const { return m_sfxMuted; }
    bool bgmMuted() const { return m_bgmMuted; }

private:
    AudioManager();
    ~AudioManager() override;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    QString soundPath(const QString& name) const;

    QString m_soundsDir;
    QString m_bgmName;

    float m_sfxVolume = 1.0f;
    float m_bgmVolume = 0.5f;
    bool m_sfxMuted = false;
    bool m_bgmMuted = false;
};

#endif // AUDIOMANAGER_H
