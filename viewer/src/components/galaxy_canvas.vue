<template>
  <canvas
   width="1024" height="576" class="canvas"
   @mousedown = "mouseDown"
  />
</template>

<script>
export default {
  props: {
    datum: {
      default: []
    }
  },
  watch: {
    datum() {
      this.draw(this.datum)
   }
  },
  methods: {
    mouseDown (e) {
      let x = ((e.offsetX / this.scale) - this.cx) >> 0;
      let y = ((e.offsetY / this.scale) - this.cy) >> 0;
      this.$emit('clicked', [x, y]);
    },
    draw(datum) {
      const scale = this.scale;

      this.ctx.beginPath();
      this.ctx.fillStyle = 'rgb(0,0,0)';
      this.ctx.fillRect(0, 0, this.$el.width, this.$el.height);

      let win_num = datum.length;
      for (let i = win_num - 1; i >= 0; i -= 1) {
        let col = ((255 / win_num) >> 0) * (i + 1);
        this.ctx.fillStyle = 'rgb(:,:,:)'.replace(/:/g, col);

        for (let j = 0; j < datum[i].length; j += 1) {
          let x = Number(datum[i][j][0]) + this.cx;
          let y = Number(datum[i][j][1]) + this.cy;
          this.ctx.fillRect(x * scale, y * scale, scale, scale);
        }
      }
    }
  },
  mounted() {
    this.ctx = this.$el.getContext('2d')
    this.scale = 4;
    this.cx = (this.$el.width / this.scale / 2) >> 0;
    this.cy = (this.$el.height / this.scale / 2) >> 0;
    
    this.draw(this.datum)
  }
}
</script>

<style scoped>
.canvas {
  border: 1px solid #000;
}
</style>