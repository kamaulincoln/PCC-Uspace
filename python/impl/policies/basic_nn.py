import baselines.common.tf_util as U
import tensorflow as tf
import gym
from baselines_master.common.distributions import make_pdtype


class BasicNNPolicy(object):
    recurrent = False

    def __init__(self, name, ob_space, ac_space):
        with tf.variable_scope(name):
            self._init(ob_space, ac_space)
            self.scope = tf.get_variable_scope().name

    def _init(self, ob_space, ac_space):
        assert isinstance(ob_space, gym.spaces.Tuple)

        self.pdtype = pdtype = make_pdtype(ac_space)
        sequence_length = None

        # ob = U.get_placeholder(name="ob", dtype=tf.float32, shape=[sequence_length] + list(ob_space.shape))
        ob = U.get_placeholder(name="ob", dtype=tf.float32, shape=[sequence_length] )

        # obscaled = ob / 255.0
        obscaled = ob


        # x = tf.nn.
        n_fields = 5
        inputs = tf.placeholder(dtype=tf.float32, shape=[1, n_fields])
        output = tf.placeholder(dtype=tf.float32, shape=[None])

        n_neurons_1 = 64
        n_neurons_2 = 32
        n_target = 1

        sigma = 1

        with tf.variable_scope("pol"):

            weight_initializer = tf.variance_scaling_initializer(mode="fan_avg", distribution="uniform", scale=sigma)
            bias_initializer = tf.zeros_initializer()

            W_hidden_1 = tf.Variable(weight_initializer([n_fields, n_neurons_1]))
            bias_hidden_1 = tf.Variable(bias_initializer([n_neurons_1]))

            W_hidden_2 = tf.Variable(weight_initializer([n_neurons_1, n_neurons_2]))
            bias_hidden_2 = tf.Variable(bias_initializer([n_neurons_2]))

            W_out = tf.Variable(weight_initializer([n_neurons_2, n_target]))
            bias_out = tf.Variable(bias_initializer([n_target]))

            hidden_1 = tf.nn.relu(tf.add(tf.matmul(inputs, W_hidden_1), bias_hidden_1))

            hidden_2 = tf.nn.relu(tf.add(tf.matmul(hidden_1, W_hidden_2), bias_hidden_2))

        with tf.variable_scope("vf"):
            weight_initializer = tf.variance_scaling_initializer(mode="fan_avg", distribution="uniform", scale=sigma)
            bias_initializer = tf.zeros_initializer()

            W_hidden_1 = tf.Variable(weight_initializer([n_fields, n_neurons_1]))
            bias_hidden_1 = tf.Variable(bias_initializer([n_neurons_1]))

            W_hidden_2 = tf.Variable(weight_initializer([n_neurons_1, n_neurons_2]))
            bias_hidden_2 = tf.Variable(bias_initializer([n_neurons_2]))

            W_out = tf.Variable(weight_initializer([n_neurons_2, n_target]))
            bias_out = tf.Variable(bias_initializer([n_target]))

            hidden_1 = tf.nn.relu(tf.add(tf.matmul(inputs, W_hidden_1), bias_hidden_1))

            hidden_2 = tf.nn.relu(tf.add(tf.matmul(hidden_1, W_hidden_2), bias_hidden_2))

        ## TODO Need to understand if we need this for each hidden layer
        # this is just a placeholder
        # logits = tf.layers.dense(hidden_1, pdtype.param_shape()[0], "logits", U.normc_initializer(0.01))
        self.pd = pdtype.pdfromflat(hidden_2)

        self.vpred = tf.transpose(tf.add(tf.matmul(hidden_2, W_out), bias_out))
        self.vpredz = self.vpred

        # mse = tf.reduce_mean(# How do I tell it what rate it should have chosen? #)
        mse = tf.reduce_mean(tf.squared_difference(self.vpred, output))

        self.state_in = []
        self.state_out = []
        #
        stochastic = tf.placeholder(dtype=tf.bool, shape=())
        # ac = self.pd.sample()
        ac = ac_space.sample()
        # self._act = U.function([stochastic, ob], [ac, self.vpred])

    def act(self, stochastic, ob):
        # ac1, vpred1 = self._act(stochastic, ob[None])
        ac1, vpred1 = self._act(stochastic, ob(0,0,0,0,0    ))

        return ac1[0], vpred1[0]

    def get_variables(self):
        return tf.get_collection(tf.GraphKeys.GLOBAL_VARIABLES, self.scope)

    def get_trainable_variables(self):
        return tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES, self.scope)

    def get_initial_state(self):
        return []

