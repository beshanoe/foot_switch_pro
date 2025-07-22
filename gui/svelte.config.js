import adapter from '@sveltejs/adapter-static';

export default {
	compilerOptions: {
		runes: true
	},
	kit: {
		adapter: adapter({
			pages: 'build',
			assets: 'build',
			fallback: 'index.html',
			precompress: false,
			strict: true
		}),
		paths: {
			base: process.argv.includes('dev') ? '' : process.env.BASE_PATH || '/foot_switch_pro'
		}
	}
};