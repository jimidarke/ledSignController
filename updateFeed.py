'''
This file is executed on a Raspberry Pi to fetch news, jokes, facts, and trivia,
and publish them to an MQTT broker, which can be used to update an LED sign.

Note: This file is not part of the main CPP project, but is used to update the feed from the Raspberry Pi or other device.

'''

import requests # type: ignore
import paho.mqtt.client as mqtt # type: ignore
import json
import time
import random
import xml.etree.ElementTree as ET

API_KEY = '/O4RGGCoHOIjK4vspbWNhA==wYHBrZMyPRkwrw6x'
RSS_FEED = "https://ctvnews.ca/rss/TopStories"
MQTT_BASE_TOPIC = "ledSign/feed"
NUM_STORIES = 5
MQTT_BROKER = "192.168.1.40"
MQTT_PORT = 1883
JOKE_API = "https://v2.jokeapi.dev/joke/Any?type=single&amount=" + str(NUM_STORIES)


triviaCategories = [
#    'artliterature',
#    'language',
    'sciencenature',
    'general',
    'fooddrink',
#    'peopleplaces',
    'geography',
#    'historyholidays',
    'entertainment',
    'toysgames',
    'music',
    'mathematics',
#    'religionmythology',
#    'sportsleisure'
]

client = mqtt.Client()

def getNews():
    response = requests.get(RSS_FEED)
    rss = response.text
    items = []
    root = ET.fromstring(rss)
    c = 0
    for item in root.findall('.//item'):
        c += 1
        title = item.find('title').text
        items.append(title)
        if c >= NUM_STORIES:
            break
    return items

def getJokes():
    response = requests.get(JOKE_API)
    jokes = response.json()
    return [joke['joke'].replace('\n',' ') for joke in jokes['jokes']]

def getFacts():
    api_url = 'https://api.api-ninjas.com/v1/facts'  
    c = 0
    facts = []
    while c < NUM_STORIES:
        response = requests.get(api_url, headers={'X-Api-Key': API_KEY})
        if response.status_code == requests.codes.ok:
            fact = response.json()
            fact = fact[0]['fact']
            facts.append(fact)
            c += 1
            time.sleep(2)
    return json.dumps(facts)
        
def getTrivia():
    #get random trivia category
    random_category = random.choice(triviaCategories)
    api_url = 'https://api.api-ninjas.com/v1/trivia?category=' + random_category
    response = requests.get(api_url, headers={'X-Api-Key': API_KEY})
    if response.status_code == requests.codes.ok:
        trivia = response.json()
        trivia = trivia[0]
        return json.dumps(trivia)
    else:
        return None

def sendToMqtt(title, message):
    # check the connection 
    if not client.is_connected():
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.publish(f"{MQTT_BASE_TOPIC}/{title}", message, retain=True)
    print(f"Published {message} to {MQTT_BASE_TOPIC}/{title}")

def main():
    facts = getFacts()
    trivia = getTrivia()
    jokes = getJokes()
    news = getNews()
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    sendToMqtt("news/list", json.dumps(news))
    sendToMqtt("jokes/list", json.dumps(jokes))
    sendToMqtt("facts/list", facts)
    sendToMqtt("trivia/question", trivia)

if __name__ == "__main__":
    main()