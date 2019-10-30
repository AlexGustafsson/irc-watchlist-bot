# IRC Watchlist Bot
### A Dockerized IRC Bot written in C and Assembly for checking messages towards various nations' watchlists.
***

### Setting up

##### Quickstart

```Bash
# Clone the repository
git clone https://github.com/AlexGustafsson/irc-watchlist-bot
# Enter the directory
cd irc-watchlist-bot
# Build the image
make docker
# Start the image
docker run -d -e IRC_SERVER='irc.example.org' --restart always axgn/irc-watchlist-bot
```

### Documentation

#### Running with Docker

```Bash
docker run -d \
-e IRC_SERVER='irc.example.org' \
-e IRC_PORT='6697' \
-e IRC_CHANNEL='#random' \
-e IRC_NICK='watchlist-bot' \
-e IRC_USER='watchlist-bot' \
-e IRC_GECOS='Watchlist Bot v0.1.0 (github.com/AlexGustafsson/irc-watchlist-bot)' \
axgn/irc-watchlist-bot
```

#### Invoking via IRC

To see help messages send `watchlist-bot: help` in the channel where the bot lives.

The bot reads all messages sent in the configured channel and sends an appropriate response if a message is on a watchlist.

### Contributing

Any contribution is welcome. If you're not able to code it yourself, perhaps someone else is - so post an issue if there's anything on your mind.

###### Development

Clone the repository:
```
git clone https://github.com/AlexGustafsson/irc-watchlist-bot && cd irc-watchlist-bot
```

Build:
```
make
```

Format code:
```
make format
```

### Disclaimer

_Although the project is very capable, it is not built with production in mind. Therefore there might be complications when trying to use the bot for large-scale projects meant for the public. The bot was created to easily check messages towards nations' watchlists in IRC channels and as such it might not promote best practices nor be performant._
