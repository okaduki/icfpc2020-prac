<template>
  <div id="app">
    <galaxy_canvas :datum="draw_datum" @clicked="clicked"/>
    <p>command:</p>
    <textarea rows="4" cols="60" placeholder="nil" v-model="command"></textarea>
    <p>_state:</p>
    <textarea rows="4" cols="60" v-model="state"></textarea>
    <p>result: {{ result }}</p>
    <button @click="exec()">exec</button>
  </div>
</template>

<script>
import galaxy_canvas from './components/galaxy_canvas'
import {eval_cmd, setup} from './parse.js'

export default {
  components: {
    galaxy_canvas
  },
  created: function() {
    setup();
  },
  data () {
    return {
      command: 'ap ap ap interact galaxy _state ap ap cons 0 0',
      result: '',
      // state: 'nil',
      state: '[5 [[2 [0 [nil [nil [nil [nil [nil [0 nil]]]]]]]] [8 [nil nil]]]]',
      draw_datum: [],
    }
  },
  methods: {
    clicked(pos) {
      this.command = 'ap ap ap interact galaxy _state ap ap cons ' + pos[0].toString() + ' ' + pos[1].toString();
      this.exec();
    },
    exec () {
      let cmd = this.command;
      let st = this.state;
      if (cmd == '') return;

      let [draws, state] = eval_cmd(cmd, st);
      this.draw_datum = draws;
      // this.result = JSON.stringify(draws, (k, v) => typeof v === "bigint" ? v.toString() + "n" : v);
      this.state = state;
    }
  }
}
</script>