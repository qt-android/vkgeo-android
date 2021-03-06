#include <QtCore/QDateTime>
#include <QtPositioning/QGeoCoordinate>
#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniObject>

#include "vkhelper.h"
#include "vkservice.h"

VKService::VKService(QObject *parent) : QObject(parent)
{
    LastUpdateFriendsTime = 0;

    connect(&UpdateFriendsTimer, &QTimer::timeout, this, &VKService::updateFriendsTimerTimeout);

    UpdateFriendsTimer.setInterval(UPDATE_FRIENDS_TIMER_INTERVAL);
    UpdateFriendsTimer.start();
}

void VKService::authStateChanged(int auth_state)
{
    if (auth_state == VKAuthState::StateNotAuthorized) {
        QtAndroid::androidService().callMethod<void>("showNotLoggedInNotification");
    } else if (auth_state == VKAuthState::StateAuthorized) {
        QtAndroid::androidService().callMethod<void>("hideNotLoggedInNotification");

        QTimer::singleShot(UPDATE_FRIENDS_ON_AUTH_DELAY, this, &VKService::updateFriendsOnAuthSingleShot);
    }
}

void VKService::dataSent()
{
    emit updateTrackedFriendsData(true);
}

void VKService::friendsUpdated()
{
    auto vk_helper = qobject_cast<VKHelper *>(QObject::sender());

    if (vk_helper != nullptr) {
        QVariantMap friends_data = vk_helper->getFriends();

        for (const QString &key : friends_data.keys()) {
            QVariantMap frnd = friends_data[key].toMap();

            if (FriendsData.contains(key)) {
                if (FriendsData[key].toMap().contains("nearby")) {
                    frnd["nearby"] = FriendsData[key].toMap()["nearby"].toBool();
                } else {
                    frnd["nearby"] = false;
                }
            } else {
                frnd["nearby"] = false;
            }

            friends_data[key] = frnd;
        }

        FriendsData = friends_data;
    }

    emit updateTrackedFriendsData(true);
}

void VKService::trackedFriendDataUpdated(const QString &id, const QVariantMap &data)
{
    auto vk_helper = qobject_cast<VKHelper *>(QObject::sender());

    if (vk_helper != nullptr) {
        if (data.contains("latitude") && data.contains("longitude")) {
            qreal latitude  = data["latitude"].toDouble();
            qreal longitude = data["longitude"].toDouble();

            if (FriendsData.contains(id)) {
                QVariantMap frnd = FriendsData[id].toMap();

                if (vk_helper->locationValid()) {
                    QGeoCoordinate my_coordinate(vk_helper->locationLatitude(), vk_helper->locationLongitude());
                    QGeoCoordinate frnd_coordinate(latitude, longitude);

                    if (my_coordinate.distanceTo(frnd_coordinate) < NEARBY_DISTANCE) {
                        if (!frnd.contains("nearby") || !frnd["nearby"].toBool()) {
                            frnd["nearby"] = true;

                            if (frnd.contains("firstName") && frnd.contains("lastName")) {
                                QAndroidJniObject j_friend_id   = QAndroidJniObject::fromString(id);
                                QAndroidJniObject j_friend_name = QAndroidJniObject::fromString(QString("%1 %2").arg(frnd["firstName"].toString())
                                                                                                                .arg(frnd["lastName"].toString()));

                                QtAndroid::androidService().callMethod<void>("showFriendsNearbyNotification", "(Ljava/lang/String;Ljava/lang/String;)V", j_friend_id.object<jstring>(),
                                                                                                                                                         j_friend_name.object<jstring>());
                            }
                        }
                    } else {
                        frnd["nearby"] = false;
                    }
                }

                FriendsData[id] = frnd;
            }
        }
    }
}

void VKService::updateFriendsOnAuthSingleShot()
{
    emit updateFriends();
}

void VKService::updateFriendsTimerTimeout()
{
    qint64 elapsed = QDateTime::currentSecsSinceEpoch() - LastUpdateFriendsTime;

    if (elapsed < 0 || elapsed > UPDATE_FRIENDS_INTERVAL) {
        LastUpdateFriendsTime = QDateTime::currentSecsSinceEpoch();

        emit updateFriends();
    }
}
