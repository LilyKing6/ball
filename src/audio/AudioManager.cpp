#include "AudioManager.h"
#include <QUrl>
#include <QDir>
#include <QDebug>
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

AudioManager& AudioManager::instance() {
    static AudioManager mgr;
    return mgr;
}

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {
    stopBgm();
}

void AudioManager::initialize(const QString& soundsDir) {
    m_soundsDir = QDir(soundsDir).absolutePath();
    qDebug() << "AudioManager initialized, sounds dir:" << m_soundsDir;
}

QString AudioManager::soundPath(const QString& name) const {
    return QDir(m_soundsDir).absoluteFilePath(name + ".wav");
}

void AudioManager::playSfx(const QString& name) {
    if (m_sfxMuted) return;
    QString path = soundPath(name);
    std::wstring wpath = path.toStdWString();
    PlaySoundW(wpath.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void AudioManager::playBgm(const QString& name) {
    if (m_bgmMuted) return;
    m_bgmName = name;
    QString path = soundPath(name);
    std::wstring wpath = path.toStdWString();
    PlaySoundW(wpath.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT);
}

void AudioManager::stopBgm() {
    PlaySoundW(nullptr, nullptr, 0);
    m_bgmName.clear();
}

void AudioManager::setSfxVolume(float v) {
    m_sfxVolume = qBound(0.0f, v, 1.0f);
    // PlaySound API doesn't support per-sound volume; store for config only
}

void AudioManager::setBgmVolume(float v) {
    m_bgmVolume = qBound(0.0f, v, 1.0f);
    // PlaySound API doesn't support per-sound volume; store for config only
}

void AudioManager::setSfxMuted(bool muted) {
    m_sfxMuted = muted;
}

void AudioManager::setBgmMuted(bool muted) {
    m_bgmMuted = muted;
    if (muted) {
        stopBgm();
    } else if (!m_bgmName.isEmpty()) {
        playBgm(m_bgmName);
    }
}
