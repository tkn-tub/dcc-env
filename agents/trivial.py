#!/usr/bin/env python3
"""
Trivial example agent for the dcc environment.
"""

import logging

import gym
import veins_gym

gym.register(
    id="veins-v1",
    entry_point="veins_gym:VeinsEnv",
    kwargs={
        "scenario_dir": "../scenario",
        "timeout": 5.0,  # increase timeout as the whole simulation needs to run
        "print_veins_stdout": True,
    },
)


def main():
    """
    Run the trivial agent.
    """
    logging.basicConfig(level=logging.DEBUG)

    env = gym.make("veins-v1")

    env.reset()
    done = False
    while not done:
        random_action = env.action_space.sample()
        observation, reward, done, info = env.step(random_action)


if __name__ == "__main__":
    main()
