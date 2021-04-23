#!/usr/bin/env python3
"""
Trivial example agent for the dcc environment.
"""

import logging

import gym
import veins_gym
from veins_gym import veinsgym_pb2


def serialize_action(actions):
    """Searialize a list of floats into an action."""
    reply = veinsgym_pb2.Reply()
    reply.action.box.values.extend(actions)
    return reply.SerializeToString()


gym.register(
    id="veins-v1",
    entry_point="veins_gym:VeinsEnv",
    kwargs={
        "scenario_dir": "../scenario",
        "timeout": 15.0,
        "print_veins_stdout": False,
        "action_serializer": serialize_action,
    },
)


def main():
    """
    Run the trivial agent.
    """
    logging.basicConfig(level=logging.DEBUG)

    env = gym.make("veins-v1")
    logging.info("Env created")

    env.reset()
    logging.info("Env reset")
    done = False
    fixed_action = [0.15, 0.15, 0.40, 0.40]
    observation, reward, done, info = env.step(fixed_action)
    while not done:
        observation, reward, done, info = env.step(fixed_action)
        logging.debug(
            "Last action: %s, Reward: %.3f, Observation: %s",
            fixed_action,
            reward,
            observation,
        )


if __name__ == "__main__":
    main()
