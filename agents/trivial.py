#!/usr/bin/env python3
"""
Trivial example agent for the dcc environment.
"""

import logging

import gym
import veins_gym
from veins_gym import veinsgym_pb2

gym.register(
    id="veins-v1",
    entry_point="veins_gym:VeinsEnv",
    kwargs={
        "scenario_dir": "../scenario",
        "timeout": 20.0,  # increase timeout as the whole simulation needs to run
        "print_veins_stdout": True,
    },
)


def _serialize_action(actions):
    logging.error("lel")
    reply = veinsgym_pb2.Reply()
    reply.action.box.values.extend(actions)
    return reply.SerializeToString()


def main():
    """
    Run the trivial agent.
    """
    logging.basicConfig(level=logging.DEBUG)

    env = gym.make("veins-v1")
    logging.info("Env created")

    setattr(env, "_serialize_action", _serialize_action)

    env.reset()
    logging.info("Env reset")
    done = False
    random_action = env.action_space.sample()
    observation, reward, done, info = env.step(random_action)
    while not done:
        random_action = env.action_space.sample()
        observation, reward, done, info = env.step(random_action)


if __name__ == "__main__":
    main()
