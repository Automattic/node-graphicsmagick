{
  'targets': [
    {
      'target_name': 'GraphicsMagick',
      'sources': [ 'src/GraphicsMagick.cc' ],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")'
      ],
      'libraries': [
        '<!@(GraphicsMagick-config --libs)',
        '<!@(GraphicsMagick-config --ldflags)'
      ],
      'conditions': [
        ['OS=="mac"', {
          # cflags on OS X are stupid and have to be defined like this
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '<!@(GraphicsMagick-config --cflags)',
              '<!@(GraphicsMagick-config --cppflags)'
            ]
          }
        }, {
          'cflags': [
            '<!@(GraphicsMagick-config --cflags)',
            '<!@(GraphicsMagick-config --cppflags)'
          ],
        }],
	['OS=="linux"', {
	  'cflags': [
	    '-fPIC'
	  ]
	}]
      ]
    }
  ]
}
