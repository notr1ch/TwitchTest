# TwitchTest
Bandwidth tester for [Twitch](https://twitch.tv/)

![Screenshot](https://i.imgur.com/uPjUImm.png)

TwitchTest creates a test stream to Twitch, using the "?bandwidthtest" option so your channel doesn't actually go live. It will attempt to stream at up to 10 mbps and show the achieved bitrate for each server. An estimation of the connection quality is presented, which is based on the number of retransmissions required and how consistent the delay between consecutive sends are. A connection quality of 80+ is recommended for a reliable stream. The TCP window size (SO_SNDBUF) can be adjusted to see what effects it has, the default setting matches what is used in [OBS](https://obsproject.com/).

Requires Administrator privileges to run, as the Windows TCP connection metrics API is only available to applications running as administrator.

A pre-compiled binary for Windows can be downloaded at [Releases](https://github.com/MrArca9/TwitchTest/releases/tag/1.3.A.0). Requires the [VS2015 x86 redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=48145#4baacbe7-a8a1-8091-5597-393c6b9ace67).
